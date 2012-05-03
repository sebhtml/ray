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

#include <plugin_GeneOntology/GeneOntology.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <core/OperatingSystem.h>
#include <plugin_GeneOntology/KeyEncoder.h>

 /* generated_automatically */
____CreateMasterModeAdapterImplementation(GeneOntology,RAY_MASTER_MODE_ONTOLOGY_MAIN); /* generated_automatically */
 /* generated_automatically */
____CreateSlaveModeAdapterImplementation(GeneOntology,RAY_SLAVE_MODE_ONTOLOGY_MAIN); /* generated_automatically */
 /* generated_automatically */
____CreateMessageTagAdapterImplementation(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS); /* generated_automatically */
____CreateMessageTagAdapterImplementation(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY); /* generated_automatically */
 /* generated_automatically */


//#define DEBUG_ONTOLOGY_SYNC
//#define DEBUG_ONTOLOGY_LOADER  //_---------------________----___---____--____------

void GeneOntology::call_RAY_MPI_TAG_SYNCHRONIZE_TERMS(Message*message){

	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] received <<call_RAY_MPI_TAG_SYNCHRONIZE_TERMS"<<endl;
	#endif

	Rank source=message->getSource();

	// we don't need to synchronize master
	if(source!=MASTER_RANK){

		int count=message->getCount();
		uint64_t*buffer=message->getBuffer();

		for(int i=0;i<count;i+=3){
			GeneOntologyIdentifier term=buffer[i+0];
			int kmerCoverage= buffer[i+1];
			int frequency=buffer[i+2];

			incrementOntologyTermFrequency(term,kmerCoverage,frequency);
		}
	}

	m_switchMan->sendMessage(NULL,0,m_outbox,m_rank,message->getSource(),
		RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY);
}

void GeneOntology::call_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY(Message*message){

}

void GeneOntology::call_RAY_MASTER_MODE_ONTOLOGY_MAIN(){
	if(!m_started){

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_started=true;


	}else if(m_switchMan->allRanksAreReady()){

		m_timePrinter->printElapsedTime("Processing gene ontologies");

		m_switchMan->closeMasterMode();
	}
}

void GeneOntology::fetchRelevantColors(){

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
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
			if(nameSpace==COLOR_NAMESPACE_EMBL_CDS){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE_MULTIPLIER;

				m_colorsForOntology.insert(colorForPhylogeny);

			}
		}
	}
		
	cout<<"Rank "<<m_rank<<" has exactly "<<m_colorsForOntology.size()<<" k-mer physical colors related to EMBL CDS objects."<<endl;
	cout<<endl;


	m_listedRelevantColors=true;

	m_loadedAnnotations=false;

}

bool GeneOntology::fetchArguments(){

	int count=m_core->getNumberOfArguments();

	char**arguments=m_core->getArgumentValues();

	string operationCode="-gene-ontology";
	int numberOfOperands=2;

	for(int i=0;i<count;i++){
		
		string token=arguments[i];

		if(token==operationCode){
	
			int remaining=count-(i+1);

			if(remaining!=numberOfOperands){
				cout<<"Error: needs "<<numberOfOperands<<" operands, you provided "<<remaining<<endl;

				return false;
			}
	
			m_ontologyFileName=arguments[i+1];
			m_annotationFileName=arguments[i+2];

			return true;
		}
	}

	return false;


}

void GeneOntology::loadAnnotations(){

	m_loadedAnnotations=true;
	m_countOntologyTermsInGraph=false;

	KeyEncoder encoder;

	if(!fetchArguments()){

		m_gotGeneOntologyParameter=false;
		return;
	}

	m_gotGeneOntologyParameter=true;

	ifstream f;
	f.open(m_annotationFileName);

	cout<<"Rank "<<m_rank<<" is loading annotations from "<<m_annotationFileName<<endl;

	int i=0;
	while(!f.eof()){
		

		if(i%500000==0){
	
			cout<<"Rank "<<m_rank<<" is loading annotations from "<<m_annotationFileName<<" ";
			cout<<i<<endl;
		}

		i++;

		string emblCdsIdentifier="";
		string goIdentifier="";

		f>>emblCdsIdentifier>>goIdentifier;

		if(emblCdsIdentifier==""||goIdentifier==""){
			continue;
		}

		PhysicalKmerColor emblCdsKey=encoder.getEncoded_EMBL_CDS(emblCdsIdentifier.c_str());

		if(m_colorsForOntology.count(emblCdsKey)==0){
	
			continue; // not in the graph anyway
		}

		GeneOntologyIdentifier goHandle=encoder.encodeGeneOntologyHandle(goIdentifier.c_str());
	
		m_annotations[emblCdsKey].push_back(goHandle);
	}

	f.close();

	cout<<"Rank "<<m_rank<<" loaded Gene Ontology annotations, ";
	cout<<m_annotations.size()<<" objects with ontology terms"<<endl;

	m_colorsForOntology.clear();
}

void GeneOntology::call_RAY_SLAVE_MODE_ONTOLOGY_MAIN(){

	if(!m_slaveStarted){

		m_listedRelevantColors=false;
		m_loadedAnnotations=false;
		m_countOntologyTermsInGraph=false;
		m_synced=false;
		m_waitingForReply=false;

		m_slaveStarted=true;

	}else if(!m_listedRelevantColors){

		fetchRelevantColors();

	}else if(!m_loadedAnnotations){

		loadAnnotations();

	}else if(!m_countOntologyTermsInGraph){
		countOntologyTermsInGraph();

	}else if(!m_synced){
		
		synchronize();

	}else{
		if(m_rank==MASTER_RANK){

			cout<<"Rank "<<m_rank<<": synchronization is complete!"<<endl;
			cout<<"Rank "<<m_rank<<": ontology terms with biological signal: "<<m_ontologyTermFrequencies.size()<<endl;

			writeOntologyFiles();
		}

		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}

string GeneOntology::getGeneOntologyName(GeneOntologyIdentifier handle){

	if(m_descriptions.count(handle)==0){

		return "NULL";
	}
	
	return m_descriptions[handle];
}

string GeneOntology::getGeneOntologyIdentifier(GeneOntologyIdentifier handle){

	if(m_identifiers.count(handle)==0){
		return "NULL";
	}

	return m_identifiers[handle];
}

void GeneOntology::writeOntologyFiles(){
	ostringstream theFile;
	theFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/_GeneOntology";

	string directory=theFile.str();
	createDirectory(directory.c_str());

	theFile<<"/Terms";

	ostringstream xmlFileStream;
	xmlFileStream<<theFile.str()<<".xml";
	ostringstream tsvFileStream;
	tsvFileStream<<theFile.str()<<".tsv";

	string xmlFile=xmlFileStream.str();
	string tsvFile=tsvFileStream.str();

	if(!m_gotGeneOntologyParameter){
		return;
	}

	loadOntology(&m_identifiers,&m_descriptions);

	map<GeneOntologyIdentifier,int> modeCoverages;
	map<GeneOntologyIdentifier,double> meanCoverages;
	map<GeneOntologyIdentifier,double> estimatedProportions;



	ofstream xmlStream(xmlFile.c_str());
	ostringstream operationBuffer; //-------------


	operationBuffer<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
	operationBuffer<<"<root>"<<endl;

	uint64_t totalForTheGraph=m_searcher->getTotalNumberOfColoredKmerObservationsForANameSpace(COLOR_NAMESPACE_EMBL_CDS);
	operationBuffer<<"<totalColoredKmerObservations>";
	operationBuffer<<totalForTheGraph<<"</totalColoredKmerObservations>"<<endl;

	for(map<GeneOntologyIdentifier,map<COVERAGE_TYPE,int> >::iterator i=
		m_ontologyTermFrequencies.begin();i!=m_ontologyTermFrequencies.end();i++){

		GeneOntologyIdentifier handle=i->first;

		int mode=0;
		int modeCount=0;
		int total=0;

		uint64_t totalObservations=0;

		for(map<COVERAGE_TYPE,int>::iterator j=i->second.begin();j!=i->second.end();j++){

			COVERAGE_TYPE coverage=j->first;
			int frequency=j->second;

			if(frequency>modeCount){
				mode=coverage;
				modeCount=frequency;
			}

			total+=frequency;

			totalObservations+=coverage*frequency;
		}

		double mean=totalObservations;

		if(total!=0){
			mean/=total;
		}
		
		#ifdef ASSERT
		assert(modeCoverages.count(handle)==0);
		assert(meanCoverages.count(handle)==0);
		#endif /**/

		modeCoverages[handle]=mode;
		meanCoverages[handle]=mean; /**/

		operationBuffer<<"<geneOntologyTerm>"<<endl;
		operationBuffer<<"<identifier>";
		operationBuffer<<getGeneOntologyIdentifier(handle)<<"</identifier><name>";
		operationBuffer<<getGeneOntologyName(handle)<<"</name>"<<endl;

		/* print paths to root */
		printPathsFromRoot(handle,&operationBuffer);

		operationBuffer<<"<modeKmerCoverage>"<<mode<<"</modeKmerCoverage>";
		operationBuffer<<"<meanKmerCoverage>"<<mean<<"</meanKmerCoverage>"<<endl;
		operationBuffer<<"<totalColoredKmerObservations>"<<totalObservations<<"</totalColoredKmerObservations>"<<endl;

		double estimatedProportion=(0.0+totalObservations);

		if(totalForTheGraph!=0){
			estimatedProportion/=totalForTheGraph;
		}
		
		estimatedProportions[handle]=estimatedProportion;

		operationBuffer<<"<proportion>"<<estimatedProportion<<"</proportion>"<<endl;
		operationBuffer<<"<distribution>"<<endl;

		operationBuffer<<"#Coverage	Frequency"<<endl;

		for(map<COVERAGE_TYPE,int>::iterator j=i->second.begin();j!=i->second.end();j++){

			COVERAGE_TYPE coverage=j->first;
			int frequency=j->second;

			operationBuffer<<coverage<<"	"<<frequency<<endl;
		}

		operationBuffer<<"</distribution></geneOntologyTerm>"<<endl;

		flushFileOperationBuffer(false,&operationBuffer,&xmlStream,CONFIG_FILE_IO_BUFFER_SIZE);
	}

	operationBuffer<<"</root>"<<endl;

	flushFileOperationBuffer(true,&operationBuffer,&xmlStream,CONFIG_FILE_IO_BUFFER_SIZE);

	xmlStream.close();

	ofstream tsvStream(tsvFile.c_str());

	operationBuffer<<"#Identifier	Name	Mode k-mer coverage	Mean k-mer coverage	Proportion"<<endl;

	for(map<GeneOntologyIdentifier,map<COVERAGE_TYPE,int> >::iterator i=
		m_ontologyTermFrequencies.begin();i!=m_ontologyTermFrequencies.end();i++){

		GeneOntologyIdentifier handle=i->first;

		operationBuffer<<getGeneOntologyIdentifier(handle)<<"	";
		operationBuffer<<getGeneOntologyName(handle)<<"	";
		operationBuffer<<modeCoverages[handle]<<"	";
		operationBuffer<<meanCoverages[handle]<<"	";
		operationBuffer<<estimatedProportions[handle]<<endl;

		flushFileOperationBuffer(false,&operationBuffer,&tsvStream,CONFIG_FILE_IO_BUFFER_SIZE);
	}

	flushFileOperationBuffer(true,&operationBuffer,&tsvStream,CONFIG_FILE_IO_BUFFER_SIZE);

	tsvStream.close();
}

void GeneOntology::loadOntology(map<GeneOntologyIdentifier,string>*identifiers,
		map<GeneOntologyIdentifier,string>*descriptions){

/* pick up all these entries:
 *
 * [Term]
 * id: GO:2001312
 * name: lysobisphosphatidic acid biosynthetic process
 */

	KeyEncoder encoder;

	char line[2048];

	ifstream f(m_ontologyFileName);

	string identifier="";
	bool processing=false;

	string typeDef="[Typedef]";

	/* is_a: GO:0007005 ! mitochondrion organization */
	string isARelation="is_a:";

	string example="GO:*******";

	while(!f.eof()){
		f.getline(line,2048);

		string overlay=line;
		if(overlay=="[Term]"){
			processing=true;

		}else if(!processing){
			// busy wait on input/output operations

		}else if(overlay.length()>=3 && overlay.substr(0,3)=="id:"){
	
			identifier=overlay.substr(4);
		}else if(overlay.length()>=5 && overlay.substr(0,5)=="name:"){
			string name=overlay.substr(6);

			#ifdef DEBUG_ONTOLOGY_LOADER
			cout<<"[DEBUG_ONTOLOGY_LOADER] "<<identifier<<" -> "<<name<<endl;
			#endif


			GeneOntologyIdentifier handle=encoder.encodeGeneOntologyHandle(identifier.c_str());

			#ifdef ASSERT
			assert(identifiers->count(handle)==0);
			assert(descriptions->count(handle)==0);
			#endif

			(*identifiers)[handle]=identifier;
			(*descriptions)[handle]=name;

		}else if(overlay.length()>=isARelation.length() && overlay.substr(0,isARelation.length())==isARelation){
		
			string parentIdentifier=overlay.substr(6,example.length());
			GeneOntologyIdentifier parentHandle=encoder.encodeGeneOntologyHandle(parentIdentifier.c_str());

			GeneOntologyIdentifier handle=encoder.encodeGeneOntologyHandle(identifier.c_str());

			addParentGeneOntologyIdentifier(handle,parentHandle);

		}else if(overlay.length()>=typeDef.length() && overlay.substr(0,typeDef.length())==typeDef){

			processing=false;
		}
	}

	f.close();

	cout<<"Rank "<<m_rank<<": loaded "<<identifiers->size()<<" gene ontology terms."<<endl;

	#ifdef ASSERT
	assert(identifiers->size()==descriptions->size());
	#endif

}

void GeneOntology::printPathsFromRoot(GeneOntologyIdentifier handle,ostream*stream){

	vector<vector<GeneOntologyIdentifier> > paths;

	getPathsFromRoot(handle,&paths);

	(*stream)<<"<paths><count>";
	(*stream)<<paths.size()<<"</count>"<<endl;

	for(int i=0;i<(int)paths.size();i++){
		(*stream)<<"<path>"<<endl;

		for(int j=0;j<(int)paths[i].size();j++){
			GeneOntologyIdentifier identifier=paths[i][j];

			(*stream)<<"<geneOntologyTerm><identifier>";
			(*stream)<<getGeneOntologyIdentifier(identifier);
			(*stream)<<"</identifier><name>";
			(*stream)<<getGeneOntologyName(identifier)<<"</name>";
			(*stream)<<"</geneOntologyTerm>"<<endl;
		}

		(*stream)<<"</path>"<<endl;
	}

	(*stream)<<"</paths>"<<endl;
}

/* returns a list of paths to the root 
 * for a given handle */
void GeneOntology::getPathsFromRoot(GeneOntologyIdentifier handle,vector<vector<GeneOntologyIdentifier> >*paths){

	// if there are no parents,
	//getParents/ simply return the simple path
	if(!hasParent(handle)){
		vector<GeneOntologyIdentifier> path;
		path.push_back(handle);
		paths->push_back(path);
		return;
	}

	// get the parents
	vector<GeneOntologyIdentifier> parents;
	getParents(handle,&parents);

	// the paths are:
	//   for each parent:
	//      path to the root of the parent (includes parent) + handle
	for(int i=0;i<(int)parents.size();i++){

		GeneOntologyIdentifier parentHandle=parents[i];
		vector<vector<GeneOntologyIdentifier> > parentPaths;
	
		getPathsFromRoot(parentHandle,&parentPaths);

		// for each of these paths,
		// add the current to the path

		for(int j=0;j<(int)parentPaths.size();j++){
			parentPaths[j].push_back(handle);

			// add this path to the final list.
			
			paths->push_back(parentPaths[j]);
		}
	}
}

void GeneOntology::addParentGeneOntologyIdentifier(GeneOntologyIdentifier term,GeneOntologyIdentifier parent){

	#ifdef ASSERT
	if(hasParent(term)){
		vector<GeneOntologyIdentifier> parents;
		getParents(term,&parents);

		for(int i=0;i<(int)parents.size();i++){
			assert(parent!=parents[i]);
		}
	}
	#endif /* ASSERT */

	m_parents[term].push_back(parent);
}

void GeneOntology::getParents(GeneOntologyIdentifier handle,vector<GeneOntologyIdentifier>*parents){

	#ifdef ASSERT
	assert(parents->size()==0);
	assert(hasParent(handle));
	#endif

	for(int i=0;i<(int)m_parents[handle].size();i++){
		parents->push_back(m_parents[handle][i]);
	}
	
}

bool GeneOntology::hasParent(GeneOntologyIdentifier handle){

	return m_parents.count(handle)>0;
}

void GeneOntology::synchronize(){

	// we received the response
	if(m_inbox->hasMessage(RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY)){

		#ifdef DEBUG_ONTOLOGY_SYNC
		cout<<"[DEBUG_ONTOLOGY_SYNC] Received RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY"<<endl;
		#endif

		m_waitingForReply=false;
	}

	if(m_waitingForReply){
		return;
	}

	// sync with master

	if(hasDataToSync()){

		#ifdef DEBUG_ONTOLOGY_SYNC
		cout<<"[DEBUG_ONTOLOGY_SYNC] Will create data message"<<endl;
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		#ifdef ASSERT
		assert(buffer!=NULL);
		#endif

		int bufferPosition=0;

		int available=(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t))/3;
		int added=0;

		while(hasDataToSync() && added < available){

			#ifdef DEBUG_ONTOLOGY_SYNC
			cout<<"[DEBUG_ONTOLOGY_SYNC] will add an object."<<endl;
			#endif

			#ifdef ASSERT
			if(added==0){
				assert(bufferPosition==0);
			}
			#endif
				
			addDataToBuffer(buffer,&bufferPosition);

			#ifdef ASSERT
			assert(bufferPosition%3 == 0);
			#endif

			added++;

			#ifdef DEBUG_ONTOLOGY_SYNC
			cout<<"[DEBUG_ONTOLOGY_SYNC] added "<<added<<" available "<<available<<endl;
			#endif
		}

		#ifdef DEBUG_ONTOLOGY_SYNC
		cout<<"[DEBUG_ONTOLOGY_SYNC] sending RAY_MPI_TAG_SYNCHRONIZE_TERMS"<<endl;
		#endif

		m_switchMan->sendMessage(buffer,bufferPosition,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_SYNCHRONIZE_TERMS);

		m_waitingForReply=true;
	}

	/* we are not waiting for a reply and we don't have more data to synchronize */
	if(!m_waitingForReply){
		m_synced=true;

		cout<<"Rank "<<m_rank<<": synced ontology term profiles with master"<<endl;
	}
}

void GeneOntology::__skipToData(){
	// skip empty slots
	while(m_ontologyTermFrequencies_iterator1 != m_ontologyTermFrequencies.end()
	&& m_ontologyTermFrequencies_iterator2==m_ontologyTermFrequencies_iterator1->second.end()){

		m_ontologyTermFrequencies_iterator1++;

		if(m_ontologyTermFrequencies_iterator1!=m_ontologyTermFrequencies.end()){
			m_ontologyTermFrequencies_iterator2=m_ontologyTermFrequencies_iterator1->second.begin();
		}
	}
}

bool GeneOntology::hasDataToSync(){

	__skipToData();

	return m_ontologyTermFrequencies_iterator1 != m_ontologyTermFrequencies.end();
}

void GeneOntology::addDataToBuffer(uint64_t*buffer,int*bufferPosition){

	GeneOntologyIdentifier term=m_ontologyTermFrequencies_iterator1->first;
	int coverage=m_ontologyTermFrequencies_iterator2->first;
	int frequency=m_ontologyTermFrequencies_iterator2->second;

	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] bufferPosition= "<<*(bufferPosition)<<endl;
	#endif

	buffer[(*bufferPosition)++]=term;
	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] bufferPosition= "<<*(bufferPosition)<<endl;
	#endif

	buffer[(*bufferPosition)++]=coverage;
	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] bufferPosition= "<<*(bufferPosition)<<endl;
	#endif

	buffer[(*bufferPosition)++]=frequency;
	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] bufferPosition= "<<*(bufferPosition)<<endl;
	#endif

	m_ontologyTermFrequencies_iterator2++;

	__skipToData();
}

void GeneOntology::countOntologyTermsInGraph(){
	
	cout<<"Rank "<<m_rank<<": counting ontology terms in the graph..."<<endl;

	#ifdef ASSERT
	assert(m_ontologyTermFrequencies.size()==0);
	#endif

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

		int kmerCoverage=node->getCoverage(&key);

		// this is the set of gene ontology terms that 
		// the current k-mer contributes to
		set<GeneOntologyIdentifier> ontologyTerms;

		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
			if(nameSpace==COLOR_NAMESPACE_EMBL_CDS){

				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE_MULTIPLIER;
	
				/* the color is in the graph, but no annotations exist... */
				if(m_annotations.count(colorForPhylogeny)==0){
					continue;
				}

				vector<GeneOntologyIdentifier>*terms=NULL;
				terms=&(m_annotations[colorForPhylogeny]);

				int numberOfTerms=terms->size();

				for(int i=0;i<numberOfTerms;i++){

					GeneOntologyIdentifier term=terms->at(i);

					ontologyTerms.insert(term);
				}
			}
		}

		// here, we have a list of gene ontology terms
		// update each of them. 

		int quantity=1;

		for(set<GeneOntologyIdentifier>::iterator i=ontologyTerms.begin();i!=ontologyTerms.end();i++){
			
			GeneOntologyIdentifier term=*i;
			incrementOntologyTermFrequency(term,kmerCoverage, quantity);
		}
	}

	m_ontologyTermFrequencies_iterator1=m_ontologyTermFrequencies.begin();

	if(m_ontologyTermFrequencies_iterator1!=m_ontologyTermFrequencies.end()){
		m_ontologyTermFrequencies_iterator2=m_ontologyTermFrequencies_iterator1->second.begin();
	}

	m_countOntologyTermsInGraph=true;

	cout<<"Rank "<<m_rank<<": "<<m_ontologyTermFrequencies.size();
	cout<<" have some biological signal"<<endl;
}

void GeneOntology::incrementOntologyTermFrequency(GeneOntologyIdentifier term,COVERAGE_TYPE kmerCoverage,int frequency){

	#ifdef ASSERT
	assert(kmerCoverage>0);
	assert(frequency>0);
	#endif

	m_ontologyTermFrequencies[term][kmerCoverage]+=frequency;

	#ifdef ASSERT
	assert(m_ontologyTermFrequencies.count(term)>0);
	assert(m_ontologyTermFrequencies[term].count(kmerCoverage)>0);
	#endif
}

void GeneOntology::registerPlugin(ComputeCore*core){

	m_core=core;

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"GeneOntology");
	core->setPluginDescription(m_plugin,"Get a ontology view on the sample.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_ONTOLOGY_MAIN=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,"RAY_MASTER_MODE_ONTOLOGY_MAIN");
	m_adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,&m_adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN);

	RAY_SLAVE_MODE_ONTOLOGY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,"RAY_SLAVE_MODE_ONTOLOGY_MAIN");
	m_adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,&m_adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN);

	RAY_MPI_TAG_ONTOLOGY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_ONTOLOGY_MAIN,"RAY_MPI_TAG_ONTOLOGY_MAIN");

	RAY_MPI_TAG_SYNCHRONIZE_TERMS=m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS,"RAY_MPI_TAG_SYNCHRONIZE_TERMS");
	m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS.setObject(this);
	m_core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS,&m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS);

	RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY=m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY,"RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY");
	m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY.setObject(this);
	m_core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY,&m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY);



	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();

}

void GeneOntology::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_ONTOLOGY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ONTOLOGY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_NEIGHBOURHOOD");

	RAY_MPI_TAG_ONTOLOGY_MAIN=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ONTOLOGY_MAIN");
	
	RAY_SLAVE_MODE_ONTOLOGY_MAIN=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ONTOLOGY_MAIN");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MPI_TAG_ONTOLOGY_MAIN);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ONTOLOGY_MAIN,RAY_SLAVE_MODE_ONTOLOGY_MAIN);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MASTER_MODE_NEIGHBOURHOOD);

	//core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MASTER_MODE_KILL_RANKS);

	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_colorSet=(ColorSet*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

	m_searcher=(Searcher*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/plugin_Searcher.ray");

	m_slaveStarted=false;
	m_started=false;
}

