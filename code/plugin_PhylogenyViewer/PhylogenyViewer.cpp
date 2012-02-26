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

#include <plugin_PhylogenyViewer/PhylogenyViewer.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <plugin_PhylogenyViewer/GenomeToTaxonLoader.h>
#include <plugin_PhylogenyViewer/PhylogeneticTreeLoader.h>
#include <plugin_PhylogenyViewer/TaxonNameLoader.h>
#include <core/OperatingSystem.h>

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

		cout<<"Global observations"<<endl;
	
		showObservations(&cout);

		ostringstream hitFile;
		hitFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Phylogeny";

		createDirectory(hitFile.str().c_str());

		hitFile<<"/KmerObservations.xml";

		ofstream f(hitFile.str().c_str());

		showObservations_XML(&f);

		f.close();

		m_timePrinter->printElapsedTime("Loading tree");

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

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

		int bufferPosition=0;

		if(!m_unknownSent){// send unknown observations too
			buffer[bufferPosition++]=UNKNOWN_TAXON;
			buffer[bufferPosition++]=m_unknown;

			m_unknownSent=true;
		}

		while(bufferPosition<maximum && m_countIterator!= m_taxonObservations.end()){
			TaxonIdentifier taxon=m_countIterator->first;
			uint64_t count=m_countIterator->second;

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
	uint64_t*buffer=message->getBuffer();

	int count=message->getCount();

	#ifdef ASSERT
	assert(count%2==0);
	assert(m_rank==MASTER_RANK);
	#endif

	for(int i=0;i<count;i+=2){
		TaxonIdentifier taxon=buffer[i];
		uint64_t count=buffer[i+1];

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
	uint64_t*buffer=message->getBuffer();

	int count=message->getCount();

	for(int i=0;i<count;i++){
		TaxonIdentifier taxon=buffer[i];

		m_taxonsForPhylogenyMaster.insert(taxon);
	}

	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_TOUCH_TAXON_REPLY);
}

void PhylogenyViewer::loadTree(){
	
	if(!m_parameters->hasOption("-with-phylogeny")){
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

			if(m_taxonsForPhylogeny.count(child) > 0){
				
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

	testPaths();

	m_loadedTree=true;

	m_gatheredObservations=false;
}

void PhylogenyViewer::gatherKmerObservations(){

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	map<int,uint64_t> frequencies;

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

		if(!nicelyAssembled){
			continue; // the k-mer is not nicely assembled...
		}

		#ifdef ASSERT
		assert(nicelyAssembled);
		#endif

		// at this point, we have a nicely assembled k-mer
		
		int kmerCoverage=node->getCoverage(&key);

		VirtualKmerColorHandle color=node->getVirtualColor();
		set<PhysicalKmerColor>*physicalColors=m_colorSet->getPhysicalColors(color);

		vector<TaxonIdentifier> taxons;

		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE;
		
			if(nameSpace==PHYLOGENY_NAMESPACE){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE;

				#ifdef ASSERT
				if(m_colorsForPhylogeny.count(colorForPhylogeny)==0){
					//cout<<"Error: color "<<colorForPhylogeny<<" should be in m_colorsForPhylogeny which contains "<<m_colorsForPhylogeny.size()<<endl;
				}

				//assert(m_colorsForPhylogeny.count(colorForPhylogeny)>0);

				if(m_genomeToTaxon.count(colorForPhylogeny)==0){

					if(m_warnings.count(colorForPhylogeny)>0){
						continue;
					}

					cout<<"Warning, color "<<colorForPhylogeny<<" is not stored, "<<m_genomeToTaxon.size()<<" available for translation:"<<endl;

					for(map<GenomeIdentifier,TaxonIdentifier>::iterator i=m_genomeToTaxon.begin();i!=m_genomeToTaxon.end();i++){
						cout<<" "<<i->first<<"->"<<i->second;
					}
					cout<<endl;

					m_warnings.insert(colorForPhylogeny);

					continue;
				}
				//assert(m_genomeToTaxon.count(colorForPhylogeny)>0);
				#endif

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
	
	cout<<endl;
	cout<<"Taxon frequencies (only one DNA strand selected)"<<endl;
	cout<<"Count	Frequency"<<endl;
	for(map<int,uint64_t>::iterator i=frequencies.begin();i!=frequencies.end();i++){
		cout<<""<<i->first<<"	"<<i->second<<endl;
	}

	cout<<"Taxon observations"<<endl;

	showObservations(&cout);

	m_gatheredObservations=true;

	m_syncedTree=false;
	m_unknownSent=false;
	m_messageReceived=true;
	m_messageSent=false;

	m_countIterator=m_taxonObservations.begin();
}

void PhylogenyViewer::showObservations_XML(ostream*stream){

	(*stream)<<"<root>"<<endl;

	(*stream)<<"<totalKmerObservations>"<<m_totalNumberOfKmerObservations<<"</totalKmerObservations>"<<endl;

	(*stream)<<"<taxon><identifier>unknown</identifier><name>unknown</name>"<<endl;
	(*stream)<<"<path>unknown</path>"<<endl;
	(*stream)<<"<kmerObservations>"<<m_unknown<<"</kmerObservations>";

	double ratio=m_unknown;
	if(m_totalNumberOfKmerObservations!=0)
		ratio/=m_totalNumberOfKmerObservations;

	(*stream)<<"<proportion>"<<ratio<<"</proportion></taxon>"<<endl;

	map<uint64_t,set<TaxonIdentifier> > sortedHits;

	// create an index to print them in a sorted way
	for(map<TaxonIdentifier,uint64_t>::iterator i=m_taxonObservations.begin();
		i!=m_taxonObservations.end();i++){

		TaxonIdentifier taxon=i->first;
		uint64_t count=i->second;

		sortedHits[count].insert(taxon);
	}

	for(map<uint64_t,set<TaxonIdentifier> >::reverse_iterator i=sortedHits.rbegin();
		i!=sortedHits.rend();i++){

		uint64_t count=i->first;

		for(set<TaxonIdentifier>::iterator j=i->second.begin();j!=i->second.end();j++){

			TaxonIdentifier taxon=*j;

			(*stream)<<"<taxon><identifier>"<<taxon<<"</identifier><name>"<<getTaxonName(taxon)<<"</name>"<<endl;
		
			(*stream)<<"<path>"<<endl;
			vector<TaxonIdentifier> path;
	
			getTaxonPathFromRoot(taxon,&path);
			printTaxonPath(taxon,&path,stream);
	
			(*stream)<<"</path>"<<endl;
			(*stream)<<"<kmerObservations>"<<count<<"</kmerObservations>";
	
			double ratio=count;
			if(m_totalNumberOfKmerObservations!=0)
				ratio/=m_totalNumberOfKmerObservations;
	
			(*stream)<<"<proportion>"<<ratio<<"</proportion></taxon>"<<endl;
	
			(*stream)<<"</taxon>"<<endl;
		}
	}

	(*stream)<<"</root>"<<endl;
}

void PhylogenyViewer::showObservations(ostream*stream){

	(*stream)<<endl;
	for(map<TaxonIdentifier,uint64_t>::iterator i=m_taxonObservations.begin();
		i!=m_taxonObservations.end();i++){

		TaxonIdentifier taxon=i->first;
		uint64_t count=i->second;

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

		for(int i=0;i<(int)taxons->size();i++){
			TaxonIdentifier taxon=taxons->at(i);

			if(m_treeParents.count(taxon)==0){
				continue;
			}

			TaxonIdentifier parent=getTaxonParent(taxon);

			parentCount[parent]++;
		}

		if(parentCount.size()==1){ // only 1 common ancestor, easy
			
			#ifdef ASSERT
			assert(parentCount.begin()->second == (int)taxons->size());
			#endif

			TaxonIdentifier taxon=parentCount.begin()->first;

			m_taxonObservations[taxon]+=kmerCoverage; // case 3.

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
			
			cout<<"Error, this is not a tree..."<<endl;
			break;
		}
	}

	return 999999999999;
}

TaxonIdentifier PhylogenyViewer::getTaxonParent(TaxonIdentifier taxon){
	if(m_treeParents.count(taxon)>0){
		return m_treeParents[taxon];
	}

	return 999999999999;
}

void PhylogenyViewer::loadTaxonNames(){

	if(!m_parameters->hasOption("-with-phylogeny")){
		return;
	}

	string file=m_parameters->getTaxonNameFile();

	TaxonNameLoader loader;

	loader.load(file);

	while(loader.hasNext()){

		TaxonIdentifier taxon;
		string name;

		loader.getNext(&taxon,&name);

		if(m_taxonsForPhylogeny.count(taxon)>0){
			m_taxonNames[taxon]=name;
		}
	}

	cout<<"Rank "<<m_rank<<" loaded taxon names from "<<file<<endl;
}

string PhylogenyViewer::getTaxonName(TaxonIdentifier taxon){
	if(m_taxonNames.count(taxon)>0){
		return m_taxonNames[taxon];
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

	while(m_treeParents.count(current)>0){
		TaxonIdentifier parent=m_treeParents[current];
	
		current=parent;

		reversePath.push_back(current);
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

	if(!m_parameters->hasOption("-with-phylogeny")){

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

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

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

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

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
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE;
		
			if(nameSpace==PHYLOGENY_NAMESPACE){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE;

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
	core->setPluginDescription(m_plugin,"Get a phylogeny view on the sample.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	m_adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,&m_adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN);

	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");
	m_adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,&m_adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN);

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,"RAY_MPI_TAG_PHYLOGENY_MAIN");

	RAY_MPI_TAG_SYNCED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCED_TAXONS,"RAY_MPI_TAG_SYNCED_TAXONS");

	RAY_MPI_TAG_LOADED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_LOADED_TAXONS,"RAY_MPI_TAG_LOADED_TAXONS");

	RAY_MPI_TAG_TOUCH_TAXON=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,"RAY_MPI_TAG_TOUCH_TAXON");
	m_adapter_RAY_MPI_TAG_TOUCH_TAXON.setObject(this);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,&m_adapter_RAY_MPI_TAG_TOUCH_TAXON);

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

	UNKNOWN_TAXON=COLOR_NAMESPACE;

	m_core=core;
}

void PhylogenyViewer::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");

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

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MASTER_MODE_KILL_RANKS);


	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_colorSet=(ColorSet*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

	m_searcher=(Searcher*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/plugin_Searcher.ray");
}

