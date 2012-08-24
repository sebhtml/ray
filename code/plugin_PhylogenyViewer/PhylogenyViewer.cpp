/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

//#define DEBUG_RECURSION

#include <plugin_PhylogenyViewer/PhylogenyViewer.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <plugin_PhylogenyViewer/GenomeToTaxonLoader.h>
#include <plugin_PhylogenyViewer/PhylogeneticTreeLoader.h>
#include <plugin_PhylogenyViewer/TaxonNameLoader.h>
#include <core/OperatingSystem.h>

__CreatePlugin(PhylogenyViewer);

 /**/
__CreateMasterModeAdapter(PhylogenyViewer,RAY_MASTER_MODE_PHYLOGENY_MAIN); /**/
 /**/
__CreateSlaveModeAdapter(PhylogenyViewer,RAY_SLAVE_MODE_PHYLOGENY_MAIN); /**/
 /**/
__CreateMessageTagAdapter(PhylogenyViewer,RAY_MPI_TAG_TOUCH_TAXON); /**/
__CreateMessageTagAdapter(PhylogenyViewer,RAY_MPI_TAG_TAXON_OBSERVATIONS); /**/
 /**/


//#define DEBUG_PHYLOGENY

void PhylogenyViewer::call_RAY_MASTER_MODE_PHYLOGENY_MAIN(){
	if(!m_started){

		#ifdef DEBUG_PHYLOGENY
		cout<<"Opening call_RAY_MASTER_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_started=true;

		m_ranksThatLoadedTaxons=0;

		m_mustSync=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TAXON_OBSERVATIONS)){

		Message*message=m_inbox->at(0);
		call_RAY_MPI_TAG_TAXON_OBSERVATIONS(message);
	
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_LOADED_TAXONS)){
		
		m_ranksThatLoadedTaxons++;

		#ifdef ASSERT
		assert(m_rank==0);
		#endif

		#ifdef DEBUG_PHYLOGENY
		cout<<"[phylogeny] m_ranksThatLoadedTaxons -> "<<m_ranksThatLoadedTaxons<<endl;
		#endif

		if(m_ranksThatLoadedTaxons==m_size){

			m_mustSync=true;
			m_ranksThatLoadedTaxons=-1;

			copyTaxonsFromSecondaryTable();

			m_taxonIterator=m_taxonsForPhylogeny.begin();

			m_responses=m_size;
			m_messageSent=false;

			cout<<"Rank "<<m_rank<<" is starting taxon syncing across the compute tribe."<<endl;

			m_timePrinter->printElapsedTime("Loading taxons");
		}

	}else if(m_mustSync){

		sendTaxonsFromMaster();


	}else if(m_switchMan->allRanksAreReady()){

		// copy synced data
		m_unknown=m_unknownMaster;
		m_taxonObservations=m_taxonObservationsMaster;

		//cout<<"Global observations"<<endl;
	
		//showObservations(&cout);

		ostringstream hitFile;
		hitFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Taxonomy";

		string directory=hitFile.str();
		//cout<<"Before createDirectory "<<directory<<endl;
		createDirectory(directory.c_str());

		hitFile<<"/Taxons.xml";

		string file=hitFile.str();
		ofstream f;
		f.open(file.c_str(),ios_base::app);

		#ifdef ASSERT
		assert(f.is_open());
		#endif

		//cout<<"Before showObservations_XML"<<endl;
		showObservations_XML(&f);

		//cout<<"Before f.close"<<endl;
		f.close();

		m_timePrinter->printElapsedTime("Loading tree");

		//cout<<"Before closeMasterMode"<<endl;
		m_switchMan->closeMasterMode();
	}
}

void PhylogenyViewer::copyTaxonsFromSecondaryTable(){
	int before=m_taxonsForPhylogeny.size();

	for(set<TaxonIdentifier>::iterator i=m_taxonsForPhylogenyMaster.begin();
		i!=m_taxonsForPhylogenyMaster.end();i++){

		m_taxonsForPhylogeny.insert(*i);
	}

	int after=m_taxonsForPhylogeny.size();

	cout<<"[PhylogenyViewer::copyTaxonsFromSecondaryTable] "<<before<<" -> "<<after<<endl;

	#ifdef ASSERT
	assert(m_taxonsForPhylogeny.size() == m_taxonsForPhylogenyMaster.size());
	#endif

	m_taxonsForPhylogenyMaster.clear();
}

void PhylogenyViewer::call_RAY_SLAVE_MODE_PHYLOGENY_MAIN(){
	if(!m_extractedColorsForPhylogeny){

		extractColorsForPhylogeny();

	}else if(!m_loadedTaxonsForPhylogeny){
	
		loadTaxons();

	}else if(!m_sentTaxonsToMaster){

		sendTaxonsToMaster();

	}else if(!m_sentTaxonControlMessage){
		
		#ifdef DEBUG_PHYLOGENY
		cout<<"Sending control message"<<endl;
		#endif

		m_switchMan->sendEmptyMessage(m_outbox,m_rank,
			MASTER_RANK,RAY_MPI_TAG_LOADED_TAXONS);

		m_sentTaxonControlMessage=true;

		m_synced=false;
	
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SYNCED_TAXONS)){
	
		copyTaxonsFromSecondaryTable();

		cout<<"Rank "<<m_rank<<" has "<<m_taxonsForPhylogeny.size()<<" taxons after syncing with master"<<endl;

		m_synced=true;
		m_loadedTree=false;

	}else if(!m_synced){

	}else if(!m_loadedTree){

		loadTree();
	
	}else if(!m_gatheredObservations){
	
		gatherKmerObservations();

	}else if(!m_syncedTree){
		
		sendTreeCounts();

	}else if(m_outbox->size()==0){

		#ifdef DEBUG_PHYLOGENY
		cout<<"Rank "<<m_rank<<" is closing call_RAY_SLAVE_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		cout<<endl;
		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}

void PhylogenyViewer::sendTreeCounts(){
	if(m_messageReceived && m_countIterator==m_taxonObservations.end()){
	
		m_syncedTree=true;

	}else if(!m_messageSent){

		#ifdef ASSERT
		assert(m_countIterator!= m_taxonObservations.end());
		#endif

		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit);

		int bufferPosition=0;

		if(!m_unknownSent){// send unknown observations too
			buffer[bufferPosition++]=UNKNOWN_TAXON;
			buffer[bufferPosition++]=m_unknown;

			m_unknownSent=true;
		}

		while(bufferPosition<maximum && m_countIterator!= m_taxonObservations.end()){
			TaxonIdentifier taxon=m_countIterator->first;
			LargeCount count=m_countIterator->second;

			buffer[bufferPosition++]=taxon;
			buffer[bufferPosition++]=count;
			m_countIterator++;
		}
		
		#ifdef ASSERT
		assert(bufferPosition!=0);
		#endif

		m_switchMan->sendMessage(buffer,bufferPosition,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_TAXON_OBSERVATIONS);

		m_messageSent=true;
		m_messageReceived=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY)){

		m_messageReceived=true;
		m_messageSent=false;
	}


}

void PhylogenyViewer::call_RAY_MPI_TAG_TAXON_OBSERVATIONS(Message*message){
	MessageUnit*buffer=message->getBuffer();

	int count=message->getCount();

	#ifdef ASSERT
	assert(count%2==0);
	assert(m_rank==MASTER_RANK);
	#endif

	for(int i=0;i<count;i+=2){
		TaxonIdentifier taxon=buffer[i];
		LargeCount count=buffer[i+1];

		if(taxon==UNKNOWN_TAXON){
			m_unknownMaster+=count;
			continue;
		}

		m_taxonObservationsMaster[taxon]+=count;
	}

	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY);
}

void PhylogenyViewer::call_RAY_MPI_TAG_TOUCH_TAXON(Message*message){
	MessageUnit*buffer=message->getBuffer();

	int count=message->getCount();

	for(int i=0;i<count;i++){
		TaxonIdentifier taxon=buffer[i];

		m_taxonsForPhylogenyMaster.insert(taxon);
	}

	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_TOUCH_TAXON_REPLY);
}

void PhylogenyViewer::loadTree(){
	
	if(!m_parameters->hasOption("-with-taxonomy")){
		m_loadedTree=true;

		return;
	}

	string treeFile=m_parameters->getTreeFile();

	PhylogeneticTreeLoader loader;

	bool growed=true;

	int iteration=0;

	while(growed){

		// we fetch everything relevant
		loader.load(treeFile);

		int oldSize=m_taxonsForPhylogeny.size();

		while(loader.hasNext()){

			TaxonIdentifier parent;
			TaxonIdentifier child;
	
			loader.getNext(&parent,&child);

			if(parent==child){
				cout<<"Error: parent and child are the same: "<<parent<<" and "<<child<<endl;
			}

			if((m_loadAllTree || (m_taxonsForPhylogeny.count(child) > 0)) && parent!=child){
				
				m_taxonsForPhylogeny.insert(parent);

				m_treeChildren[parent].insert(child);
	
				m_treeParents[child]=parent;
			}
		}

		int newSize=m_taxonsForPhylogeny.size();

		growed=(newSize > oldSize);

		cout<<"Rank "<<m_rank<<" "<<"loadTree iteration= "<<iteration<<" oldSize= "<<oldSize<<" newSize= "<<newSize<<endl;

		iteration++;
	}

	// load taxonNames
	loadTaxonNames();

	// don't test paths in production
	//testPaths();

	m_loadedTree=true;

	m_gatheredObservations=false;
}

void PhylogenyViewer::gatherKmerObservations(){

	/* set to true to use only assembled kmers */
	bool useOnlyAssembledKmer=false;

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	map<CoverageDepth,LargeCount> frequencies;

	while(iterator.hasNext()){

		#ifdef ASSERT
		assert(parity==0 || parity==1);
		#endif

		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());
		
		if(parity==0){
			parity=1;
		}else if(parity==1){
			parity=0;

			continue; // we only need data with parity=0
		}

		// check for assembly paths

		/* here, we just want to find a path with
		* a good progression */

		Direction*a=node->m_directions;
		bool nicelyAssembled=false;

		while(a!=NULL){
			int progression=a->getProgression();

			if(progression>= CONFIG_NICELY_ASSEMBLED_KMER_POSITION){
				nicelyAssembled=true;
			}

			a=a->getNext();
		}

		if(useOnlyAssembledKmer && !nicelyAssembled){
			continue; // the k-mer is not nicely assembled...
		}

		#ifdef ASSERT
		assert(nicelyAssembled || !useOnlyAssembledKmer);
		#endif

		// at this point, we have a nicely assembled k-mer
		
		int kmerCoverage=node->getCoverage(&key);

		VirtualKmerColorHandle color=node->getVirtualColor();
		set<PhysicalKmerColor>*physicalColors=m_colorSet->getPhysicalColors(color);

		vector<TaxonIdentifier> taxons;

		// get a list of taxons associated with this kmer
		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			PhysicalKmerColor nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
			if(nameSpace==COLOR_NAMESPACE_PHYLOGENY){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE_MULTIPLIER;

				#ifdef ASSERT
				if(m_colorsForPhylogeny.count(colorForPhylogeny)==0){
					//cout<<"Error: color "<<colorForPhylogeny<<" should be in m_colorsForPhylogeny which contains "<<m_colorsForPhylogeny.size()<<endl;
				}
				#endif

				//assert(m_colorsForPhylogeny.count(colorForPhylogeny)>0);

				// this means that this genome is not in the taxonomy tree
				if(m_genomeToTaxon.count(colorForPhylogeny)==0){

					if(m_warnings.count(colorForPhylogeny)==0){
						cout<<"Warning, color "<<colorForPhylogeny<<" is not stored, "<<m_genomeToTaxon.size()<<" available. This means that you provided a genome sequence that is not classified in the taxonomy."<<endl;

						#ifdef VERBOSE
						for(map<GenomeIdentifier,TaxonIdentifier>::iterator i=m_genomeToTaxon.begin();i!=m_genomeToTaxon.end();i++){
							cout<<" "<<i->first<<"->"<<i->second;
						}
						cout<<endl;
						#endif
					}

					m_warnings.insert(colorForPhylogeny);

					continue;
				}

				#ifdef ASSERT
				assert(m_genomeToTaxon.count(colorForPhylogeny)>0);
				#endif

				TaxonIdentifier taxon=m_genomeToTaxon[colorForPhylogeny];

				taxons.push_back(taxon);
			}
		}

		classifySignal(&taxons,kmerCoverage,node,&key);

		int count=taxons.size();

		frequencies[count]++;
	}
	
/*
 *
 * TODO: move this in colored operation files
 *
	cout<<endl;
	cout<<"Taxon frequencies (only one DNA strand selected)"<<endl;
	cout<<"Count	Frequency"<<endl;
	for(map<CoverageDepth,LargeCount>::iterator i=frequencies.begin();i!=frequencies.end();i++){
		cout<<""<<i->first<<"	"<<i->second<<endl;
	}

	cout<<"Taxon observations"<<endl;
*/

	//showObservations(&cout);

	m_gatheredObservations=true;

	m_syncedTree=false;
	m_unknownSent=false;
	m_messageReceived=true;
	m_messageSent=false;

	m_countIterator=m_taxonObservations.begin();
}

LargeCount PhylogenyViewer::getSelfCount(TaxonIdentifier taxon){
	if(m_taxonObservations.count(taxon)==0){
		return 0;
	}
	
	return m_taxonObservations[taxon];
}

void PhylogenyViewer::populateRanks(map<string,LargeCount>*rankSelfObservations,
		map<string,LargeCount>*rankRecursiveObservations){

	for(map<TaxonIdentifier,string>::iterator i=m_taxonNames.begin();
		i!=m_taxonNames.end();i++){

		TaxonIdentifier taxon=i->first;
		LargeCount selfCount=getSelfCount(taxon);
		LargeCount recursiveCount=getRecursiveCount(taxon);

		string rank=getTaxonRank(taxon);

		if(rankRecursiveObservations->count(rank)==0){
			(*rankRecursiveObservations)[rank]=0;
		}
		(*rankRecursiveObservations)[rank]+=recursiveCount;

		if(rankSelfObservations->count(rank)==0){
			(*rankSelfObservations)[rank]=0;
		}
		(*rankSelfObservations)[rank]+=selfCount;
	}
}

void PhylogenyViewer::showObservations_XML(ostream*stream){

	ostringstream operationBuffer;

	/* build a mashup for the ranks
 * this will contain total at each level */

	map<string,LargeCount> rankRecursiveObservations;
	map<string,LargeCount> rankSelfObservations;

	populateRanks(&rankSelfObservations,&rankRecursiveObservations);

	operationBuffer<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
	operationBuffer<<"<root>"<<endl;

	/* add the sample name in the XML file */
	operationBuffer<<"<sample>";
	operationBuffer<<m_parameters->getSampleName();
	operationBuffer<<"</sample>"<<endl;

	operationBuffer<<"<totalAssembledKmerObservations>"<<m_totalNumberOfKmerObservations<<"</totalAssembledKmerObservations>"<<endl;

	LargeCount totalColoredAssembledKmerObservations=m_totalNumberOfKmerObservations-m_unknown;

	operationBuffer<<"<totalColoredAssembledKmerObservations>"<<totalColoredAssembledKmerObservations<<"</totalColoredAssembledKmerObservations>"<<endl;

	operationBuffer<<"<ranks>"<<endl;

	for(map<string,LargeCount>::iterator i=rankSelfObservations.begin();i!=rankSelfObservations.end();i++){
		string rank=i->first;

		#ifdef ASSERT
		assert(rankRecursiveObservations.count(rank)>0);
		assert(rankSelfObservations.count(rank)>0);
		#endif

		operationBuffer<<"<entry><rank>"<<rank<<"</rank><self><kmerObservations>";
		operationBuffer<<rankSelfObservations[rank]<<"</kmerObservations></self>";
		operationBuffer<<"<recursive><kmerObservations>"<<rankRecursiveObservations[rank];
		operationBuffer<<"</kmerObservations></recursive></entry>"<<endl;

	}

	operationBuffer<<"</ranks>"<<endl;

	operationBuffer<<"<entry>";
	operationBuffer<<"<taxon><identifier>unknown</identifier><name>unknown</name><rank>unknown</rank></taxon>"<<endl;
	operationBuffer<<"<path></path>"<<endl;
	operationBuffer<<"<self><kmerObservations>"<<m_unknown<<"</kmerObservations>";

	double ratio=m_unknown;

	if(m_totalNumberOfKmerObservations!=0)
		ratio/=m_totalNumberOfKmerObservations;

	operationBuffer<<"<proportion>"<<ratio<<"</proportion>";
	operationBuffer<<"<coloredProportion>0</coloredProportion>";
	operationBuffer<<"<coloredProportionInRank>0</coloredProportionInRank></self></entry>"<<endl;

	// declare tsv files
	map<string,FILE*> tsvFiles;
	map<string,ostringstream*> tsvBuffers;

	for(map<TaxonIdentifier,string>::iterator i=m_taxonNames.begin();
		i!=m_taxonNames.end();i++){

		TaxonIdentifier taxon=i->first;

		LargeCount count=getSelfCount(taxon);

		string rank=getTaxonRank(taxon);

		#ifdef ASSERT
		assert(rankSelfObservations.count(rank)>0);
		assert(rankRecursiveObservations.count(rank)>0);
		#endif

		LargeCount rankRecursiveCount=rankRecursiveObservations[rank];

		#ifdef ASSERT
		LargeCount rankSelfCount=rankSelfObservations[rank]; //-

		assert(rankSelfCount>=0);
		assert(rankRecursiveCount>=0);
		#endif

		LargeCount recursiveCount=getRecursiveCount(taxon);

		if(recursiveCount==0){
			continue;
		}

		operationBuffer<<"<entry>"<<endl;

		printTaxon_XML(taxon,&operationBuffer);

		vector<TaxonIdentifier> path;

		getTaxonPathFromRoot(taxon,&path);
		printTaxonPath_XML(taxon,&path,&operationBuffer);

		operationBuffer<<"<self>"<<endl;
		operationBuffer<<"<kmerObservations>"<<count<<"</kmerObservations>";


		double ratio=count;
		if(m_totalNumberOfKmerObservations!=0)
			ratio/=m_totalNumberOfKmerObservations;

		operationBuffer<<"<proportion>"<<ratio<<"</proportion>";

		double coloredRatio=count;

		if(totalColoredAssembledKmerObservations!=0){
			coloredRatio/=totalColoredAssembledKmerObservations;
		}

		operationBuffer<<"<coloredProportion>"<<coloredRatio<<"</coloredProportion>";

		operationBuffer<<"</self>"<<endl;

		operationBuffer<<"<recursive>";
		operationBuffer<<"<kmerObservations>";
		operationBuffer<<recursiveCount;
		operationBuffer<<"</kmerObservations>"<<endl;


		double ratio2=recursiveCount;
		if(m_totalNumberOfKmerObservations!=0)
			ratio2/=m_totalNumberOfKmerObservations;

		operationBuffer<<"<proportion>"<<ratio2<<"</proportion>";

		double coloredRatio2=recursiveCount;

		if(totalColoredAssembledKmerObservations!=0){
			coloredRatio2/=totalColoredAssembledKmerObservations;
		}

		operationBuffer<<"<coloredProportion>"<<coloredRatio2<<"</coloredProportion>";

		double coloredRatioInRank=recursiveCount;

		if(rankRecursiveCount!=0){
			coloredRatioInRank/=rankRecursiveCount;
		}

		operationBuffer<<"<coloredProportionInRank>"<<coloredRatioInRank<<"</coloredProportionInRank>";
		
		operationBuffer<<"</recursive>"<<endl;

		operationBuffer<<"</entry>"<<endl;

		flushFileOperationBuffer(false,&operationBuffer,stream,CONFIG_FILE_IO_BUFFER_SIZE);

		// add data to the tsv file


		if(tsvFiles.count(rank)==0){
			ostringstream theFile;
			theFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
			theFile<<"0.Profile.TaxonomyRank="<<rank<<".tsv";
	
			string tsvFile=theFile.str();
			tsvFiles[rank]=fopen(tsvFile.c_str(),"a");

			tsvBuffers[rank]=new ostringstream();

			*(tsvBuffers[rank])<<"#TaxonIdentifier	TaxonName	TaxonRank	TaxonProportion"<<endl;
		}

		string name=getTaxonName(taxon);

		*(tsvBuffers[rank])<<taxon<<"	"<<name<<"	"<<rank;
		*(tsvBuffers[rank])<<"	"<<coloredRatioInRank<<endl;
	}

	// close XML files
	operationBuffer<<"</root>"<<endl;
	flushFileOperationBuffer(true,&operationBuffer,stream,CONFIG_FILE_IO_BUFFER_SIZE);


	// close tsv files
	for(map<string,FILE*>::iterator i=tsvFiles.begin();i!=tsvFiles.end();i++){
	
		string rank=i->first;
		FILE*file=i->second;

		string text=tsvBuffers[rank]->str();
		fprintf(file,"%s",text.c_str());

		delete tsvBuffers[rank];

		fclose(file);
	}

	tsvBuffers.clear();
	tsvFiles.clear();

}

LargeCount PhylogenyViewer::getRecursiveCount(TaxonIdentifier taxon){

	if(m_taxonRecursiveObservations.count(taxon)>0){

		#ifdef DEBUG_RECURSION
		cout<<"[fast return] for taxon "<<getTaxonName(taxon)<<" value= ";
		cout<<m_taxonRecursiveObservations[taxon]<<endl;
		#endif

		return m_taxonRecursiveObservations[taxon];
	}

	LargeCount count=getSelfCount(taxon);

	if(m_treeChildren.count(taxon)>0){
		for(set<TaxonIdentifier>::iterator i=m_treeChildren[taxon].begin();
			i!=m_treeChildren[taxon].end();i++){
			
			TaxonIdentifier child=*i;

			count+=getRecursiveCount(child);
		}
	}

	m_taxonRecursiveObservations[taxon]=count;

	return count;
}

void PhylogenyViewer::showObservations(ostream*stream){

	(*stream)<<endl;
	for(map<TaxonIdentifier,LargeCount>::iterator i=m_taxonObservations.begin();
		i!=m_taxonObservations.end();i++){

		TaxonIdentifier taxon=i->first;
		LargeCount count=i->second;

		(*stream)<<endl;
		(*stream)<<"Taxon: "<<getTaxonName(taxon)<<" ["<<taxon<<"]"<<endl;
		(*stream)<<" path: ";
		vector<TaxonIdentifier> path;

		getTaxonPathFromRoot(taxon,&path);
		printTaxonPath(taxon,&path,stream);

		(*stream)<<" k-mer observations: "<<count<<endl;
	}

	(*stream)<<endl;
	(*stream)<<"Taxon: Unknown"<<endl;
	(*stream)<<" path: / ???"<<endl;
	(*stream)<<" k-mer observations: "<<m_unknown<<endl;
}

void PhylogenyViewer::classifySignal(vector<TaxonIdentifier>*taxons,int kmerCoverage,Vertex*vertex,Kmer*key){
	// given a list of taxon,
	// place the kmer coverage somewhere in
	// the tree
	//
	// case 1.
	// if there are 0 taxons, this is unknown stuff
	//
	// case 2.
	// if there is one taxon, place the coverage on it
	//
	// case 3.
	// if there is at least 2 taxons and they all have the same parent
	//
	// case 4.
	// if there is at least 2 taxons and they don't have the same parent
	//  but they have a common ancestor
	


	if(taxons->size()==0){

		m_unknown+=kmerCoverage; // case 1.

	}else if(taxons->size()==1){
		TaxonIdentifier taxon=taxons->at(0);

		m_taxonObservations[taxon]+=kmerCoverage; // case 2.

	}else{ // more than 1

		#ifdef ASSERT
		assert(taxons->size()>1);
		#endif

		// a taxon can only have one parent,
		// simply check if they have all the same parent...

		map<TaxonIdentifier,int> parentCount;

		int found=0;

		for(int i=0;i<(int)taxons->size();i++){
			TaxonIdentifier taxon=taxons->at(i);

			if(m_treeParents.count(taxon)==0){
				
				cout<<"Warning: Taxon "<<taxon<<" is not in the tree"<<endl;
				continue;
			}

			TaxonIdentifier parent=getTaxonParent(taxon);

			parentCount[parent]++;
			found++;
		}

		if(parentCount.size()==1){ // only 1 common ancestor, easy
			
			#ifdef ASSERT
			if(!(parentCount.begin()->second == found)){
				cout<<"Error: taxons: "<<taxons->size()<<", parentCount: "<<parentCount.size()<<" 1 element with "<<parentCount.begin()->second<<" taxons"<<endl;
			}
			assert(parentCount.begin()->second == found);
			#endif

			TaxonIdentifier taxon=parentCount.begin()->first;

			m_taxonObservations[taxon]+=kmerCoverage; // case 3.

			return;
		}

		if(parentCount.size()==0){
			cout<<"Error, no parents, returning now."<<endl;
			return;
		}

		// at this point, we have more than one taxon and
		// they don't share the same parent

		// since we have a tree, find the nearest common ancestor
		// in the worst case, the common ancestor is the root

		#ifdef ASSERT
		assert(parentCount.size()>1);
		#endif

		TaxonIdentifier taxon=findCommonAncestor(taxons);

		// classify it
		m_taxonObservations[taxon]+=kmerCoverage; // case 4.
	}
}

TaxonIdentifier PhylogenyViewer::findCommonAncestor(vector<TaxonIdentifier>*taxons){

	map<TaxonIdentifier,int> counts;

	// for a deep tree, this algorithm would not be really fast.
	// so instead of computing path to the root,
	// the algorithm should stop when one ancestor is found

	vector<TaxonIdentifier> currentTaxons;

	for(int i=0;i<(int)taxons->size();i++){
		TaxonIdentifier taxon=taxons->at(i);
		currentTaxons.push_back(taxon);
	}

	while(1){
		int blocked=0;

		// compute each path to the root
		for(int i=0;i<(int)currentTaxons.size();i++){
			TaxonIdentifier taxon=currentTaxons[i];

			// no more parents
			if(m_treeParents.count(taxon)==0){
				blocked++;
				continue;
			}

			TaxonIdentifier parent=m_treeParents[taxon];

			counts[parent]++;

			// we found a common ancestor
			// this is the deepest
			if(counts[parent]==(int)currentTaxons.size()){ 
				return parent;
			}
			
			currentTaxons[i]=parent; // update it
		}

		if(blocked==(int)currentTaxons.size()){ // this will happen if it is not a tree
			
			cout<<"Error, this is not a tree, blocked: "<<blocked<<" currentTaxons: "<<currentTaxons.size()<<endl;
			break;
		}
	}

	return 999999999999ULL;
}

TaxonIdentifier PhylogenyViewer::getTaxonParent(TaxonIdentifier taxon){
	if(m_treeParents.count(taxon)>0){
		return m_treeParents[taxon];
	}

	return 999999999999ULL;
}

void PhylogenyViewer::loadTaxonNames(){

	if(!m_parameters->hasOption("-with-taxonomy")){
		return;
	}

	string file=m_parameters->getTaxonNameFile();

	TaxonNameLoader loader;

	loader.load(file);

	while(loader.hasNext()){

		TaxonIdentifier taxon;
		string name;
		string rank;

		loader.getNext(&taxon,&name,&rank);

		if(m_taxonsForPhylogeny.count(taxon)>0){
			m_taxonNames[taxon]=name;
			m_taxonRanks[taxon]=rank;
		}
	}

	cout<<"Rank "<<m_rank<<" loaded taxon names from "<<file<<endl;
}

string PhylogenyViewer::getTaxonName(TaxonIdentifier taxon){
	if(m_taxonNames.count(taxon)>0){
		string value=m_taxonNames[taxon];
		
		for(int i=0;i<(int)value.length();i++){

			char symbol=value[i];

			/* < and > are not allowed in XML files unless they are un tag names */
			if(symbol=='<' || symbol=='>'){
				value[i]='_';
			}
		}

		return value; /* return sane taxon name */
	}

	return "CachingError";
}

void PhylogenyViewer::testPaths(){
	cout<<endl;
	cout<<"[PhylogenyViewer::testPaths]"<<endl;

	for(set<TaxonIdentifier>::iterator i=m_taxonsForPhylogeny.begin();i!=m_taxonsForPhylogeny.end();i++){
		TaxonIdentifier taxon=*i;

		vector<TaxonIdentifier> path;

		getTaxonPathFromRoot(taxon,&path);

		cout<<endl;

		printTaxonPath(taxon,&path,&cout);
	}
	cout<<endl;
}

void PhylogenyViewer::printTaxonPath_XML(TaxonIdentifier taxon,vector<TaxonIdentifier>*path,ostream*stream){

	(*stream)<<"<path>"<<endl;
	//cout<<"Taxon= "<<taxon<<endl;

	for(int i=0;i<(int)path->size();i++){

		TaxonIdentifier taxon2=(*path)[i];

		printTaxon_XML(taxon2,stream);
	}

	(*stream)<<"</path>"<<endl;
}

void PhylogenyViewer::printTaxon_XML(TaxonIdentifier taxon,ostream*stream){
	string name=getTaxonName(taxon);
	string rank=getTaxonRank(taxon);

	(*stream)<<"<taxon><name>"<<name<<"</name><rank>"<<rank<<"</rank><identifier>"<<taxon<<"</identifier></taxon>"<<endl;
}

string PhylogenyViewer::getTaxonRank(TaxonIdentifier taxon){
	if(m_taxonRanks.count(taxon)==0){
		return "CachingError";
	}

	return m_taxonRanks[taxon];
}

void PhylogenyViewer::printTaxonPath(TaxonIdentifier taxon,vector<TaxonIdentifier>*path,ostream*stream){

	//cout<<"Taxon= "<<taxon<<endl;

	for(int i=0;i<(int)path->size();i++){

		TaxonIdentifier taxon=(*path)[i];
		(*stream)<<" / "<<getTaxonName(taxon)<<" ["<<taxon<<"] ";
	}
	(*stream)<<endl;

}

void PhylogenyViewer::getTaxonPathFromRoot(TaxonIdentifier taxon,vector<TaxonIdentifier>*path){

	TaxonIdentifier current=taxon;

	vector<TaxonIdentifier> reversePath;

	reversePath.push_back(current);

	int maximum=100;

	while(m_treeParents.count(current)>0){
		TaxonIdentifier parent=m_treeParents[current];
	
		current=parent;

		reversePath.push_back(current);

		if((int)reversePath.size()==maximum){
			cout<<"Warning: path is too long with "<<reversePath.size()<<" vertices."<<endl;
			break;
		}
	}

	int i=reversePath.size()-1;

	while(i>=0){
		TaxonIdentifier current=reversePath[i];
		path->push_back(current);

		i--;
	}
}

/*
 * Loads taxons by fetching pairs in a conversion file.
 */
void PhylogenyViewer::loadTaxons(){

	if(!m_parameters->hasOption("-with-taxonomy")){

		m_loadedTaxonsForPhylogeny=true;
		m_sentTaxonsToMaster=false;
		m_taxonIterator=m_taxonsForPhylogeny.begin();
		m_messageSent=false;
		m_messageReceived=true;

		return;
	}

	string genomeToTaxonFile=m_parameters->getGenomeToTaxonFile();

	GenomeToTaxonLoader genomeToTaxonUnit;

	genomeToTaxonUnit.load(genomeToTaxonFile);

	while(genomeToTaxonUnit.hasNext()){
	
		GenomeIdentifier genome;
		TaxonIdentifier taxon;

		genomeToTaxonUnit.getNext(&genome,&taxon);

		if(m_colorsForPhylogeny.count(genome)>0){
			m_taxonsForPhylogeny.insert(taxon);
			
			m_genomeToTaxon[genome]=taxon;
		}
	}

	cout<<"Rank "<<m_rank<<" loaded "<<m_taxonsForPhylogeny.size()<<" taxons."<<endl;

	m_colorsForPhylogeny.clear();

	m_loadedTaxonsForPhylogeny=true;
	m_sentTaxonsToMaster=false;
	m_taxonIterator=m_taxonsForPhylogeny.begin();
	m_messageSent=false;
	m_messageReceived=true;
}

void PhylogenyViewer::sendTaxonsFromMaster(){

	if(m_responses == m_size && m_taxonIterator==m_taxonsForPhylogeny.end()){

		m_switchMan->sendToAll(m_outbox,m_rank,RAY_MPI_TAG_SYNCED_TAXONS);

		cout<<"Rank "<<m_rank<<" synced taxons across the grid with "<<m_size<<" poor slaves."<<endl;

		m_mustSync=false;

	}else if(!m_messageSent){

		#ifdef ASSERT
		assert(m_taxonIterator!= m_taxonsForPhylogeny.end());
		#endif

		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit);

		int bufferPosition=0;

		while(bufferPosition<maximum && m_taxonIterator!= m_taxonsForPhylogeny.end()){
			TaxonIdentifier taxon=*m_taxonIterator;
			buffer[bufferPosition]=taxon;
			bufferPosition++;
			m_taxonIterator++;
		}
		
		#ifdef ASSERT
		assert(bufferPosition!=0);
		#endif

		m_switchMan->sendMessageToAll(buffer,bufferPosition,m_outbox,m_rank,RAY_MPI_TAG_TOUCH_TAXON);

		m_messageSent=true;
		m_responses=0;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TOUCH_TAXON_REPLY)){

		m_responses++;

		if(m_responses==m_size){
	
			m_messageSent=false;
		}
	}
}



void PhylogenyViewer::sendTaxonsToMaster(){

	if(m_messageReceived && m_taxonIterator==m_taxonsForPhylogeny.end()){

		m_sentTaxonsToMaster=true;
	
		m_sentTaxonControlMessage=false;

	}else if(!m_messageSent){

		#ifdef ASSERT
		assert(m_taxonIterator!= m_taxonsForPhylogeny.end());
		#endif

		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit);

		int bufferPosition=0;

		while(bufferPosition<maximum && m_taxonIterator!= m_taxonsForPhylogeny.end()){
			TaxonIdentifier taxon=*m_taxonIterator;
			buffer[bufferPosition]=taxon;
			bufferPosition++;
			m_taxonIterator++;
		}
		
		#ifdef ASSERT
		assert(bufferPosition!=0);
		#endif

		m_switchMan->sendMessage(buffer,bufferPosition,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_TOUCH_TAXON);

		m_messageSent=true;
		m_messageReceived=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TOUCH_TAXON_REPLY)){

		m_messageReceived=true;
		m_messageSent=false;
	}
}

/**
 * here we extract the phylogeny colors
 */
void PhylogenyViewer::extractColorsForPhylogeny(){

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	while(iterator.hasNext()){

		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());

		#ifdef ASSERT
		assert(parity==0 || parity==1);
		#endif

		if(parity==0){
			parity=1;
		}else if(parity==1){
			parity=0;

			continue; // we only need data with parity=0
		}

		VirtualKmerColorHandle color=node->getVirtualColor();
		set<PhysicalKmerColor>*physicalColors=m_colorSet->getPhysicalColors(color);

		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			PhysicalKmerColor nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
			if(nameSpace==COLOR_NAMESPACE_PHYLOGENY){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE_MULTIPLIER;

				m_colorsForPhylogeny.insert(colorForPhylogeny);

				#ifdef DEBUG_PHYLOGENY
				cout<<"[phylogeny] colorForPhylogeny= "<<colorForPhylogeny<<endl;
				#endif
			}
		}
	}
		
	cout<<"Rank "<<m_rank<<" has exactly "<<m_colorsForPhylogeny.size()<<" k-mer physical colors for the phylogeny."<<endl;
	cout<<endl;

	m_extractedColorsForPhylogeny=true;

	m_loadedTaxonsForPhylogeny=false;

	m_totalNumberOfKmerObservations=m_searcher->getTotalNumberOfKmerObservations();
}

void PhylogenyViewer::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"PhylogenyViewer");
	core->setPluginDescription(m_plugin,"Get a taxonomy view of the sample.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,__GetAdapter(PhylogenyViewer,RAY_MASTER_MODE_PHYLOGENY_MAIN));

	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,__GetAdapter(PhylogenyViewer,RAY_SLAVE_MODE_PHYLOGENY_MAIN));

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,"RAY_MPI_TAG_PHYLOGENY_MAIN");

	RAY_MPI_TAG_SYNCED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCED_TAXONS,"RAY_MPI_TAG_SYNCED_TAXONS");

	RAY_MPI_TAG_LOADED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_LOADED_TAXONS,"RAY_MPI_TAG_LOADED_TAXONS");

	RAY_MPI_TAG_TOUCH_TAXON=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,"RAY_MPI_TAG_TOUCH_TAXON");
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,__GetAdapter(PhylogenyViewer,RAY_MPI_TAG_TOUCH_TAXON));

	RAY_MPI_TAG_TOUCH_TAXON_REPLY=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TOUCH_TAXON_REPLY,"RAY_MPI_TAG_TOUCH_TAXON_REPLY");

	RAY_MPI_TAG_TAXON_OBSERVATIONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TAXON_OBSERVATIONS,"RAY_MPI_TAG_TAXON_OBSERVATIONS");

	RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY,"RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY");

	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();
	m_extractedColorsForPhylogeny=false;

	UNKNOWN_TAXON=COLOR_NAMESPACE_MULTIPLIER;

	m_loadAllTree=true;

	m_core=core;
}

void PhylogenyViewer::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_NEIGHBOURHOOD");
	RAY_MASTER_MODE_ONTOLOGY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ONTOLOGY_MAIN");

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PHYLOGENY_MAIN");
	RAY_MPI_TAG_TOUCH_TAXON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TOUCH_TAXON");
	RAY_MPI_TAG_TOUCH_TAXON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TOUCH_TAXON_REPLY");
	RAY_MPI_TAG_LOADED_TAXONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LOADED_TAXONS");
	RAY_MPI_TAG_SYNCED_TAXONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SYNCED_TAXONS");
	RAY_MPI_TAG_TAXON_OBSERVATIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TAXON_OBSERVATIONS");
	RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY");
	
	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MPI_TAG_PHYLOGENY_MAIN);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,RAY_SLAVE_MODE_PHYLOGENY_MAIN);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MASTER_MODE_ONTOLOGY_MAIN);

	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_colorSet=(ColorSet*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

	m_searcher=(Searcher*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/plugin_Searcher.ray");

	m_started=false;

	__BindPlugin(PhylogenyViewer);
}

