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

#define SENTINEL_VALUE_FOR_TOTAL 123456789

#include <plugin_GeneOntology/GeneOntology.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <core/OperatingSystem.h>
#include <plugin_GeneOntology/KeyEncoder.h>

__CreatePlugin(GeneOntology);

 /**/
__CreateMasterModeAdapter(GeneOntology,RAY_MASTER_MODE_ONTOLOGY_MAIN); /**/
 /**/
__CreateSlaveModeAdapter(GeneOntology,RAY_SLAVE_MODE_ONTOLOGY_MAIN); /**/
 /**/
__CreateMessageTagAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS); /**/
__CreateMessageTagAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY); /**/
__CreateMessageTagAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZATION_DONE); /**/
 /**/

//#define BUG_DETERMINISM
//#define DEBUG_ONTOLOGY_SYNC
//#define DEBUG_ONTOLOGY_LOADER  //_---------------________----___---____--____------

void GeneOntology::call_RAY_MPI_TAG_SYNCHRONIZE_TERMS(Message*message){

	Rank source=message->getSource();

	#ifdef DEBUG_ONTOLOGY_SYNC
	cout<<"[DEBUG_ONTOLOGY_SYNC] received call_RAY_MPI_TAG_SYNCHRONIZE_TERMS from "<<source<<endl;
	#endif

	// we don't need to synchronize master
	if(source!=MASTER_RANK){

		int count=message->getCount();
		MessageUnit*buffer=message->getBuffer();

		for(int i=0;i<count;i+=3){

			GeneOntologyIdentifier term=buffer[i+0];
			CoverageDepth kmerCoverage= buffer[i+1];
			LargeCount frequency=buffer[i+2];

			if(term==SENTINEL_VALUE_FOR_TOTAL && kmerCoverage == SENTINEL_VALUE_FOR_TOTAL
				&& count==3){

				LargeCount kmerObservationsWithGeneOntologies=frequency;

				m_kmerObservationsWithGeneOntologies+=kmerObservationsWithGeneOntologies;

				cout<<"Rank "<<m_rank;
				cout<<" Synchronizing kmer observations with gene ontology terms: ";
				cout<<kmerObservationsWithGeneOntologies;
				cout<<" (from "<<source<<")"<<endl;
		
				continue;
			}

			#ifdef BUG_DETERMINISM
			if(term==49){
				cout<<"[BUG_DETERMINISM] viaMessage incrementOntologyTermFrequency "<<term<<" "<<kmerCoverage<<" "<<frequency<<endl;
			}
			#endif

			incrementOntologyTermFrequency(term,kmerCoverage,frequency);
		}
	}

	m_switchMan->sendMessage(NULL,0,m_outbox,m_rank,message->getSource(),
		RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY);
}

string GeneOntology::getDomainName(GeneOntologyDomain handle){
	if(m_domainNames.size()==0){
		m_domainNames[GENE_ONTOLOGY_DOMAIN_molecular_function]=GENE_ONTOLOGY_DOMAIN_molecular_function_STRING;
		m_domainNames[GENE_ONTOLOGY_DOMAIN_cellular_component]=GENE_ONTOLOGY_DOMAIN_cellular_component_STRING;
		m_domainNames[GENE_ONTOLOGY_DOMAIN_biological_process]=GENE_ONTOLOGY_DOMAIN_biological_process_STRING;
	}

	#ifdef ASSERT
	assert(m_domainNames.count(handle)==1);
	#endif

	return m_domainNames[handle];
}

GeneOntologyDomain GeneOntology::getGeneOntologyDomain(const char*text){

	if(m_domains.size()==0){
		m_domains[GENE_ONTOLOGY_DOMAIN_molecular_function_STRING]=GENE_ONTOLOGY_DOMAIN_molecular_function;
		m_domains[GENE_ONTOLOGY_DOMAIN_cellular_component_STRING]=GENE_ONTOLOGY_DOMAIN_cellular_component;
		m_domains[GENE_ONTOLOGY_DOMAIN_biological_process_STRING]=GENE_ONTOLOGY_DOMAIN_biological_process;
	}

	#ifdef ASSERT
	assert(m_domains.count(text)>0);
	#endif

	return m_domains[text];
}

void GeneOntology::call_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY(Message*message){

}

void GeneOntology::call_RAY_MPI_TAG_SYNCHRONIZATION_DONE(Message*message){

	m_ranksSynchronized++;
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
	
			PhysicalKmerColor nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
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

	string operationCode="-gene-ontology";
	int numberOfOperands=2;

	if(m_parameters->hasConfigurationOption(operationCode.c_str(),numberOfOperands)){

		int offset=0;
		m_ontologyFileName=m_parameters->getConfigurationString(operationCode.c_str(),offset++);
		m_annotationFileName=m_parameters->getConfigurationString(operationCode.c_str(),offset++);

		cout<<"[GeneOntology] ontology file: "<<m_ontologyFileName<<", annotation file: ";
		cout<<m_annotationFileName<<endl;

		ifstream test1(m_ontologyFileName.c_str());
		bool test1Result=test1;
		test1.close();

		if(!test1Result){
			cout<<"Error: the file "<<m_ontologyFileName<<" does not exist."<<endl;
			return false;
		}

		ifstream test2(m_annotationFileName.c_str());
		bool test2Result=test2;
		test2.close();

		if(!test2Result){
			cout<<"Error: the file "<<m_annotationFileName<<" does not exist."<<endl;
			return false;
		}

		return true;
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
	f.open(m_annotationFileName.c_str());

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

		loadOntology(&m_identifiers,&m_descriptions);

	}else if(!m_countOntologyTermsInGraph){

		countOntologyTermsInGraph();

	}else if(!m_synced){
		
		synchronize();

	}else{
		if(m_rank==MASTER_RANK){

			// busy wait for other to complete their tasks.
			if(m_ranksSynchronized<m_core->getMessagesHandler()->getSize()){
				return;
			}

			cout<<"Rank "<<m_rank<<": synchronization is complete!"<<endl;
			cout<<"Rank "<<m_rank<<": ontology terms with biological signal: "<<m_ontologyTermFrequencies.size()<<endl;

			writeOntologyFiles();

			writeTrees();
		}

		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}

void GeneOntology::writeTrees(){

	populateRecursiveValues();

	// compute depths
	computeDepths(m_cellularComponentHandle,0);
	computeDepths(m_molecularFunctionHandle,0);
	computeDepths(m_biologicalProcessHandle,0);

	int withoutDepth=0;

	for(map<GeneOntologyIdentifier,GeneOntologyDomain>::iterator i=m_termDomains.begin();
		i!=m_termDomains.end();i++){

		GeneOntologyIdentifier handle=i->first;

		if(m_depths.count(handle)==0){
			withoutDepth++;
		}
	}

	cout<<"Gene ontology terms without depth information: "<<withoutDepth<<endl;


	writeOntologyProfile(GENE_ONTOLOGY_DOMAIN_biological_process);
	writeOntologyProfile(GENE_ONTOLOGY_DOMAIN_cellular_component);
	writeOntologyProfile(GENE_ONTOLOGY_DOMAIN_molecular_function);
}

void GeneOntology::writeOntologyProfile(GeneOntologyDomain domain){

	int maximumDepth=getDomainDepth(domain);

	cout<<"[GeneOntology] maximum depth for GeneOntologyDomain "<<domain<<" is "<<maximumDepth<<endl;

	string domainName="NULL";

	if(domain==GENE_ONTOLOGY_DOMAIN_biological_process){
		domainName=GENE_ONTOLOGY_DOMAIN_biological_process_STRING;
	}else if(domain==GENE_ONTOLOGY_DOMAIN_cellular_component){
		domainName=GENE_ONTOLOGY_DOMAIN_cellular_component_STRING;
	}else if(domain==GENE_ONTOLOGY_DOMAIN_molecular_function){
		domainName=GENE_ONTOLOGY_DOMAIN_molecular_function_STRING;
	}

	#ifdef ASSERT
	assert(domainName!="NULL");
	#endif

	LargeCount totalForTheGraph=m_searcher->getTotalNumberOfColoredKmerObservationsForANameSpace(COLOR_NAMESPACE_EMBL_CDS);

	for(int depth=0;depth<maximumDepth;depth++){

		// create the file for the domain and given depth.

		ostringstream operationBuffer;

		ostringstream fileName;
		fileName<<m_parameters->getPrefix()<<"/BiologicalAbundances/_GeneOntology";
		fileName<<"/"<<domainName<<".Depth="<<depth<<".tsv";
		string file2=fileName.str();

		ofstream file;


		for(map<GeneOntologyIdentifier,int>::iterator i=m_recursiveCounts.begin();
			i!=m_recursiveCounts.end();i++){

			GeneOntologyIdentifier handle=i->first;

			int count=i->second;


			if(count==0){
				continue;
			}

			if(!hasDepth(handle)){
				continue;
			}

			if(getGeneOntologyDepth(handle)!=depth){
				continue;
			}

			if(getDomain(handle)!=domain){
				continue;
			}

			double proportion=count;

			if(totalForTheGraph!=0){
				proportion/=totalForTheGraph;
			}

			if(!file.is_open()){
				file.open(file2.c_str());
				operationBuffer<<"#Identifier	Name	Proportion	Observations	Total"<<endl;
			}

			operationBuffer<<getGeneOntologyIdentifier(handle);
			operationBuffer<<"	"<<getGeneOntologyName(handle)<<"	";
			operationBuffer<<proportion;
			operationBuffer<<"	"<<count<<"	"<<totalForTheGraph<<endl;

			flushFileOperationBuffer(false,&operationBuffer,&file,CONFIG_FILE_IO_BUFFER_SIZE);

		}

		if(file.is_open()){
			flushFileOperationBuffer(true,&operationBuffer,&file,CONFIG_FILE_IO_BUFFER_SIZE);
			file.close();
		}
	}
}

bool GeneOntology::hasDepth(GeneOntologyIdentifier handle){

	return m_depths.count(handle)==1;
}

int GeneOntology::getGeneOntologyDepth(GeneOntologyIdentifier handle){

	#ifdef ASSERT
	if(m_depths.count(handle)==0){
		cout<<"Error: handle "<<handle<<" identifier="<<getGeneOntologyIdentifier(handle)<<" has no depth"<<endl;
	}

	assert(m_depths.count(handle)==1);
	#endif

	return m_depths[handle];
}

int GeneOntology::getDomainDepth(GeneOntologyDomain domain){

	GeneOntologyIdentifier root=m_molecularFunctionHandle;

	if(domain==GENE_ONTOLOGY_DOMAIN_molecular_function){
		root=m_molecularFunctionHandle;
	}else if(domain==GENE_ONTOLOGY_DOMAIN_cellular_component){
		root=m_cellularComponentHandle;
	}else if(domain==GENE_ONTOLOGY_DOMAIN_biological_process){
		root=m_biologicalProcessHandle;
	}

	return getDeepestDepth(root,0);
}

int GeneOntology::getDeepestDepth(GeneOntologyIdentifier handle,int depth){

	bool skipDifferentDomain=true;

	vector<GeneOntologyIdentifier> children;
	getChildren(handle,&children);

	int deepest=depth;

	for(int i=0;i<(int)children.size();i++){

		GeneOntologyIdentifier child=children[i];
		
		if(skipDifferentDomain && getDomain(child)!=getDomain(handle)){

			continue;
		}

		int newDepth=getDeepestDepth(child,depth+1);

		if(newDepth>deepest){
			deepest=newDepth;
		}
	}

	return deepest;
}

void GeneOntology::populateRecursiveValues(){

	for(map<GeneOntologyIdentifier,GeneOntologyDomain>::iterator i=m_termDomains.begin();
		i!=m_termDomains.end();i++){

		GeneOntologyIdentifier handle=i->first;

		set<GeneOntologyIdentifier> visited;

		int recursiveCount=computeRecursiveCount(handle,&visited);

		addRecursiveCount(handle,recursiveCount);
	}

	cout<<"Populated recursive values..."<<endl;
}

void GeneOntology::addRecursiveCount(GeneOntologyIdentifier handle,int count){

	#ifdef ASSERT
	assert(m_recursiveCounts.count(handle)==0);
	#endif

	m_recursiveCounts[handle]=count;
}

int GeneOntology::getRecursiveCount(GeneOntologyIdentifier handle){
	#ifdef ASSERT
	assert(m_recursiveCounts.count(handle)==1);
	#endif

	return m_recursiveCounts[handle];

}

void GeneOntology::computeDepths(GeneOntologyIdentifier handle,int depth){

	if(m_depths.count(depth)==1){
		return;
	}

	m_depths[handle]=depth;

	bool skipDifferentDomain=true;

	vector<GeneOntologyIdentifier> children;

	getChildren(handle,&children);

	for(int i=0;i<(int)children.size();i++){

		GeneOntologyIdentifier child=children[i];
		
		if(skipDifferentDomain && getDomain(child)!=getDomain(handle)){

			continue;
		}

		computeDepths(child,depth+1);
	}
}


int GeneOntology::computeRecursiveCount(GeneOntologyIdentifier handle,set<GeneOntologyIdentifier>*visited){

	bool skipDifferentDomain=true;

	int count=0;

	// don't count any term twice
	if(visited->count(handle)>1){
		return count;
	}
	
	count+=getGeneOntologyCount(handle);

	visited->insert(handle);

	vector<GeneOntologyIdentifier> children;
	getChildren(handle,&children);

	for(int i=0;i<(int)children.size();i++){

		GeneOntologyIdentifier child=children[i];
		
		if(skipDifferentDomain && getDomain(child)!=getDomain(handle)){
			continue;
		}

		count+=computeRecursiveCount(child,visited);
	}

	return count;
}

int GeneOntology::getGeneOntologyCount(GeneOntologyIdentifier handle){

	if(m_termCounts.count(handle)==0){
		return 0;
	}

	return m_termCounts[handle];
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


	map<GeneOntologyIdentifier,int> modeCoverages;
	map<GeneOntologyIdentifier,double> meanCoverages;
	map<GeneOntologyIdentifier,double> estimatedProportions;



	ofstream xmlStream(xmlFile.c_str());
	ostringstream operationBuffer; //-------------

	cout<<"TOTAL: "<<m_kmerObservationsWithGeneOntologies<<endl;

	operationBuffer<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
	operationBuffer<<"<root>"<<endl;

	LargeCount totalForTheGraph=m_searcher->getTotalNumberOfColoredKmerObservationsForANameSpace(COLOR_NAMESPACE_EMBL_CDS);
	operationBuffer<<"<totalColoredKmerObservations>";
	operationBuffer<<totalForTheGraph<<"</totalColoredKmerObservations>"<<endl;

	// declare tsv files
	map<string,FILE*> tsvFiles;
	map<string,ostringstream*> tsvBuffers;



	for(map<GeneOntologyIdentifier,map<CoverageDepth,int> >::iterator i=
		m_ontologyTermFrequencies.begin();i!=m_ontologyTermFrequencies.end();i++){

		GeneOntologyIdentifier handle=i->first;

		int mode=0;
		int modeCount=0;
		int total=0;

		LargeCount totalObservations=0;

		for(map<CoverageDepth,int>::iterator j=i->second.begin();j!=i->second.end();j++){

			CoverageDepth coverage=j->first;
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

		GeneOntologyDomain domain=getDomain(handle);
		string domainName=getDomainName(domain);

		operationBuffer<<"<geneOntologyTerm>"<<endl;
		operationBuffer<<"<identifier>";
		operationBuffer<<getGeneOntologyIdentifier(handle)<<"</identifier><name>";
		operationBuffer<<getGeneOntologyName(handle)<<"</name>"<<endl;
		operationBuffer<<"<domain>"<<domainName<<"</domain>"<<endl;

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

		m_termCounts[handle]=totalObservations;

		operationBuffer<<"<proportion>"<<estimatedProportion<<"</proportion>"<<endl;
		operationBuffer<<"<distribution>"<<endl;

		operationBuffer<<"#Coverage	Frequency"<<endl;

		for(map<CoverageDepth,int>::iterator j=i->second.begin();j!=i->second.end();j++){

			CoverageDepth coverage=j->first;
			int frequency=j->second;

			operationBuffer<<coverage<<"	"<<frequency<<endl;
		}

		operationBuffer<<"</distribution></geneOntologyTerm>"<<endl;

		flushFileOperationBuffer(false,&operationBuffer,&xmlStream,CONFIG_FILE_IO_BUFFER_SIZE);


		// also output beautiful tsv file too


		if(tsvFiles.count(domainName)==0){
			ostringstream theFile;
			theFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
			theFile<<"0.Profile.GeneOntologyDomain="<<domainName<<".tsv";
	
			string tsvFile=theFile.str();
			tsvFiles[domainName]=fopen(tsvFile.c_str(),"a");

			tsvBuffers[domainName]=new ostringstream();

			*(tsvBuffers[domainName])<<"#TermIdentifier	TermName	TermDomain	TermProportion"<<endl;
		}

		*(tsvBuffers[domainName])<<getGeneOntologyIdentifier(handle)<<"	";
		*(tsvBuffers[domainName])<<getGeneOntologyName(handle)<<"	";
		*(tsvBuffers[domainName])<<domainName;
		*(tsvBuffers[domainName])<<"	"<<estimatedProportion<<endl;

	}

	operationBuffer<<"</root>"<<endl;

	flushFileOperationBuffer(true,&operationBuffer,&xmlStream,CONFIG_FILE_IO_BUFFER_SIZE);

	xmlStream.close();

	ofstream tsvStream(tsvFile.c_str());

	operationBuffer<<"#Identifier	Name	Mode k-mer coverage	Mean k-mer coverage	Proportion"<<endl;


	// close tsv files
	for(map<string,FILE*>::iterator i=tsvFiles.begin();i!=tsvFiles.end();i++){
	
		string category=i->first;
		FILE*file=i->second;

		string text=tsvBuffers[category]->str();
		fprintf(file,"%s",text.c_str());

		delete tsvBuffers[category];

		fclose(file);
	}

	tsvBuffers.clear();
	tsvFiles.clear();



	for(map<GeneOntologyIdentifier,map<CoverageDepth,int> >::iterator i=
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

void GeneOntology::setDomain(GeneOntologyIdentifier handle,GeneOntologyDomain domain){
	
	#ifdef ASSERT
	assert(m_termDomains.count(handle)==0);
	#endif

	m_termDomains[handle]=domain;
}

GeneOntologyDomain GeneOntology::getDomain(GeneOntologyIdentifier handle){

	// if the term has no domain, return molecular_function
	// this should not happen anyway because alternative identifiers
	// are dereferenced.
	if(m_termDomains.count(handle)==0){
		cout<<"Error, GeneOntologyIdentifier "<<handle<<" has no domain"<<endl;
		return GENE_ONTOLOGY_DOMAIN_molecular_function;
	}

	#ifdef ASSERT
	if(m_termDomains.count(handle)==0){
		cout<<"Error, handle= "<<handle<<" termDomains: "<<m_termDomains.size()<<endl;
	}

	assert(m_termDomains.count(handle)>0);
	#endif

	return m_termDomains[handle];
}

void GeneOntology::loadOntology(map<GeneOntologyIdentifier,string>*identifiers,
		map<GeneOntologyIdentifier,string>*descriptions){

	if(!m_gotGeneOntologyParameter){
		return ; /*--*/
	}

/* pick up all these entries:
 *
 * [Term]
 * id: GO:2001312
 * name: lysobisphosphatidic acid biosynthetic process
 */

	KeyEncoder encoder;

	char line[2048];

	ifstream f(m_ontologyFileName.c_str());

	string identifier="";
	bool processing=false;

	string typeDef="[Typedef]";

	/* is_a: GO:0007005 ! mitochondrion organization */
	string isARelation="is_a:";

	string example="GO:*******";

	string theNamespace="namespace:";
	string alternate="alt_id: ";

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

			if(name==GENE_ONTOLOGY_DOMAIN_biological_process_STRING){
				m_biologicalProcessHandle=handle;
			}else if(name==GENE_ONTOLOGY_DOMAIN_molecular_function_STRING){
				m_molecularFunctionHandle=handle;
			}else if(name==GENE_ONTOLOGY_DOMAIN_cellular_component_STRING){
				m_cellularComponentHandle=handle;
			}

		}else if(overlay.length()>=theNamespace.length() && overlay.substr(0,theNamespace.length())==theNamespace){

			string namespaceName=overlay.substr(theNamespace.length()+1);

			GeneOntologyDomain domain=getGeneOntologyDomain(namespaceName.c_str());

			GeneOntologyIdentifier handle=encoder.encodeGeneOntologyHandle(identifier.c_str());

			setDomain(handle,domain);
		
		}else if(overlay.length()>=alternate.length() && overlay.substr(0,alternate.length())==alternate){

			string alternateIdentifier=overlay.substr(alternate.length());

			GeneOntologyIdentifier alternateHandle=encoder.encodeGeneOntologyHandle(alternateIdentifier.c_str());

			GeneOntologyIdentifier handle=encoder.encodeGeneOntologyHandle(identifier.c_str());

			// an alternate handle can be utilised only once
			#ifdef ASSERT
			assert(m_symbolicLinks.count(alternateHandle)==0);
			#endif

			m_symbolicLinks[alternateHandle]=handle;

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

	m_children[parent].push_back(term);
}

void GeneOntology::getChildren(GeneOntologyIdentifier handle,vector<GeneOntologyIdentifier>*children){

	#ifdef ASSERT
	assert(children->size()==0);
	#endif

	if(m_children.count(handle)==0){
		return;
	}

	for(int i=0;i<(int)m_children[handle].size();i++){
		children->push_back(m_children[handle][i]);
	}
	
}

void GeneOntology::getParents(GeneOntologyIdentifier handle,vector<GeneOntologyIdentifier>*parents){

	#ifdef ASSERT
	assert(parents->size()==0);
	#endif

	if(m_parents.count(handle)==0){
		return;
	}

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

	bool isMaster=m_core->getMessagesHandler()->getRank() == MASTER_RANK;

	if(!m_synchronizedTotal && !isMaster){
	
		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferPosition=0;

		m_synchronizedTotal=true;

		buffer[bufferPosition++]=SENTINEL_VALUE_FOR_TOTAL;
		buffer[bufferPosition++]=SENTINEL_VALUE_FOR_TOTAL;
		buffer[bufferPosition++]=m_kmerObservationsWithGeneOntologies;

		m_switchMan->sendMessage(buffer,bufferPosition,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_SYNCHRONIZE_TERMS);

		m_waitingForReply=true;

		return;

	}

	// sync with master

	if(hasDataToSync() && !isMaster){

		#ifdef DEBUG_ONTOLOGY_SYNC
		cout<<"[DEBUG_ONTOLOGY_SYNC] Will create data message"<<endl;
		#endif

		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		#ifdef ASSERT
		assert(buffer!=NULL);
		#endif

		int bufferPosition=0;

		int available=(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit))/3;
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

		m_switchMan->sendMessage(NULL,0,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_SYNCHRONIZATION_DONE);
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

void GeneOntology::addDataToBuffer(MessageUnit*buffer,int*bufferPosition){

	GeneOntologyIdentifier term=m_ontologyTermFrequencies_iterator1->first;
	CoverageDepth coverage=m_ontologyTermFrequencies_iterator2->first;
	LargeCount frequency=m_ontologyTermFrequencies_iterator2->second;

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
	
	m_kmerObservationsWithGeneOntologies=0;

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
	
			PhysicalKmerColor nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
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

			GeneOntologyIdentifier realTerm=dereferenceTerm(term);

			#ifdef BUG_DETERMINISM
			if(term==49){
				cout<<"[BUG_DETERMINISM] viaCounter: incrementOntologyTermFrequency "<<term<<" "<<kmerCoverage<<" "<<quantity<<endl;
			}
			#endif

			incrementOntologyTermFrequency(realTerm,kmerCoverage, quantity);
		}

		// update the total
		if(!ontologyTerms.empty()){
			m_kmerObservationsWithGeneOntologies+=kmerCoverage;
		}
	}

	m_ontologyTermFrequencies_iterator1=m_ontologyTermFrequencies.begin();

	if(m_ontologyTermFrequencies_iterator1!=m_ontologyTermFrequencies.end()){
		m_ontologyTermFrequencies_iterator2=m_ontologyTermFrequencies_iterator1->second.begin();
	}

	m_countOntologyTermsInGraph=true;

	cout<<"Rank "<<m_rank<<": "<<m_ontologyTermFrequencies.size();
	cout<<" have some biological signal"<<endl;
	cout<<"Number of dereferenced alternate handles: "<<m_dereferences<<endl;
	cout<<"Number of k-mer observations with gene ontology terms: ";
	cout<<m_kmerObservationsWithGeneOntologies<<endl;
}

GeneOntologyIdentifier GeneOntology::dereferenceTerm(GeneOntologyIdentifier handle){

	set<GeneOntologyIdentifier> visited;

	return dereferenceTerm_safe(handle,&visited);
}

GeneOntologyIdentifier GeneOntology::dereferenceTerm_safe(GeneOntologyIdentifier handle,set<GeneOntologyIdentifier>*visited){

	if(m_symbolicLinks.count(handle)==0 || visited->count(handle) == 1){
		return handle;
	}

	visited->insert(handle);
	m_dereferences++;

	GeneOntologyIdentifier link=m_symbolicLinks[handle];

	return dereferenceTerm_safe(link,visited);
}

void GeneOntology::incrementOntologyTermFrequency(GeneOntologyIdentifier term,CoverageDepth kmerCoverage,int frequency){

	#ifdef ASSERT
	assert(kmerCoverage>0);
	assert(frequency>0);

	// make sure we are not using an alternate identifier...
	assert(m_termDomains.count(term)==1);
	assert(m_descriptions.count(term)==1);
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
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,__GetAdapter(GeneOntology,RAY_MASTER_MODE_ONTOLOGY_MAIN));

	RAY_SLAVE_MODE_ONTOLOGY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,"RAY_SLAVE_MODE_ONTOLOGY_MAIN");
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,__GetAdapter(GeneOntology,RAY_SLAVE_MODE_ONTOLOGY_MAIN));

	RAY_MPI_TAG_ONTOLOGY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_ONTOLOGY_MAIN,"RAY_MPI_TAG_ONTOLOGY_MAIN");

	RAY_MPI_TAG_SYNCHRONIZATION_DONE=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCHRONIZATION_DONE,"RAY_MPI_TAG_SYNCHRONIZATION_DONE");
	m_core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SYNCHRONIZATION_DONE,__GetAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZATION_DONE));

	RAY_MPI_TAG_SYNCHRONIZE_TERMS=m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS,"RAY_MPI_TAG_SYNCHRONIZE_TERMS");
	m_core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS,__GetAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS));

	RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY=m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY,"RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY");
	m_core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY,__GetAdapter(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY));



	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();

	m_synchronizedTotal=false;

	m_ranksSynchronized=0;
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

	__BindPlugin(GeneOntology);
}

