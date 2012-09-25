/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

/* 
 * TODO: add option -minimumScaffoldLength
 */

#include <core/OperatingSystem.h>
#include<application_core/common_functions.h>
#include<application_core/Parameters.h>
#include <plugin_Library/LibraryPeakFinder.h>
#include<plugin_SequencesLoader/Read.h>
#include<plugin_SequencesLoader/Loader.h>
#include <memory/MyAllocator.h>

#include<string>
#include<sstream>
#include<iostream>
#include<vector>
#include<cstdlib>
#include<fstream>
#include<assert.h>
#include<math.h>
using namespace std;

void Parameters::getIndexes(int count,vector<int>*out){
	vector<int> numbers;
	for(int i=0;i<count;i++)
		numbers.push_back(i);

	srand(99);
	while(numbers.size()>0){
		int randomIndex=rand()%numbers.size();
		int index=numbers[randomIndex];
		out->push_back(index);
		vector<int> newNumbers;
		for(int i=0;i<(int)numbers.size();i++){
			if(randomIndex==i)
				continue;
			newNumbers.push_back(numbers[i]);
		}
		numbers=newNumbers;
	}
}

Parameters::Parameters(){
	m_providedPeakCoverage=false;
	m_providedRepeatCoverage=false;
	m_providedMinimumCoverage=false;
	m_prefix="RayOutput";
	m_initiated=false;
	m_showMemoryAllocations=false;
	m_directory="assembly";
	m_minimumContigLength=100;
	m_wordSize=21;
	m_colorSpaceMode=false;
	m_reducerIsActivated=false;
	m_amos=false;
	m_error=false;
	m_memoryFilePrefix=m_prefix;
	m_profiler=false;
	m_debugBubbles=false;
	m_debugSeeds=false;
	m_showMemoryUsage=true;

	m_showEndingContext=false;
	m_writeKmers=false;
	m_showExtensionChoice=false;
	m_showReadPlacement=false;

	/** use the new NovaEngine (TM) */
	m_options.insert("-use-NovaEngine");

	/** the default is to not use a default value */
	m_degree=2;

	m_checkpointDirectory="Checkpoints";
	m_hasCheckpointDirectory=false;

	m_maximumSeedCoverage=getMaximumAllowedCoverage();
}

bool Parameters::showExtensionChoice(){
	return m_showExtensionChoice;
}

bool Parameters::showEndingContext(){
	return m_showEndingContext;
}

bool Parameters::debugBubbles(){
	return m_debugBubbles;
}

bool Parameters::runProfiler(){
	return m_profiler;
}

bool Parameters::debugSeeds(){
	return m_debugSeeds;
}

int Parameters::getWordSize(){
	return m_wordSize;
}

void Parameters::loadCommandsFromFile(char*file){

	// first, load the configuration into a stream
	
	ostringstream content;
	#define __BLOCK_SIZE 4096

	ifstream f(file);
	while(!f.eof()){
		char buffer[__BLOCK_SIZE];

		f.read(buffer,__BLOCK_SIZE);

		int i=0;
		int characterRead=f.gcount();

		while(i<characterRead)
			content<<buffer[i++];
	}
	f.close();

	#undef __BLOCK_SIZE

	ostringstream withoutComments;

	m_configurationContent=content.str();

	bool insideComment=false;

	int total=m_configurationContent.length();

	int i=0;
	char beginComment='#';
	char endComment='\n';

	while(i<total){
		char data=m_configurationContent[i];

		if(data==beginComment && !insideComment){
			insideComment=true;
		}else if(data==endComment && insideComment){
			insideComment=false;
		}else if(!insideComment){
			withoutComments<<data;
		}
		i++;
	}

	string withoutCommentsString=withoutComments.str();

	//#define __debug_comments

	#ifdef __debug_comments
	cout<<"<Original>"<<endl;
	cout<<m_configurationContent;
	cout<<"</Original>"<<endl;

	cout<<"<Without comments>"<<endl;
	cout<<withoutCommentsString;
	cout<<"</Without comments>"<<endl;

	#endif

	istringstream realData(withoutCommentsString);

	while(1){
		string token="";

		realData>>token;

		if(token=="")
			break;

		m_commands.push_back(token);
	}
	
}

void Parameters::loadCommandsFromArguments(int argc,char**argv){
	for(int i=1;i<argc;i++){
		m_commands.push_back(argv[i]);
	}
}

/* parse commands */
void Parameters::parseCommands(){

	m_initiated=true;
	set<string> commands;

	for(int i=0;i<(int)m_commands.size();i++)
		m_options.insert(m_commands[i]);

	m_showCommunicationEvents=false;

	if(hasOption("-show-communication-events"))
		m_showCommunicationEvents=true;

/*
	if(hasOption("-test-network-only")){
		m_options.insert("-write-network-test-raw-data");
	}
*/

	if(hasOption("-show-read-placement")){
		m_showReadPlacement=true;
	}

	m_originalCommands=m_commands;

	#if 0

	__shuffleOperationCodes();

	#endif

	set<string> singleReadsCommands;
	singleReadsCommands.insert("-s");
	singleReadsCommands.insert("LoadSingleEndReads");
	singleReadsCommands.insert("-LoadSingleEndReads");
	singleReadsCommands.insert("--LoadSingleEndReads");

	set<string> pairedReadsCommands;
	pairedReadsCommands.insert("-p");
	pairedReadsCommands.insert("LoadPairedEndReads");
	pairedReadsCommands.insert("-LoadPairedEndReads");
	pairedReadsCommands.insert("--LoadPairedEndReads");
	
	set<string> interleavedCommands;
	interleavedCommands.insert("-i");

	set<string> colorSpaceMode;
	colorSpaceMode.insert("-color-space");
	set<string> outputAmosCommands;
	outputAmosCommands.insert("-a");
	outputAmosCommands.insert("-amos");
	outputAmosCommands.insert("--amos");
	outputAmosCommands.insert("--output-amos");
	outputAmosCommands.insert("-OutputAmosFile");
	outputAmosCommands.insert("--OutputAmosFile");

	set<string> showMalloc;
	showMalloc.insert("-show-memory-allocations");

	set<string> outputFileCommands;
	outputFileCommands.insert("-o");
	outputFileCommands.insert("-OutputFile");
	outputFileCommands.insert("--OutputFile");
	
	set<string> memoryMappedFileCommands;
	memoryMappedFileCommands.insert("-MemoryPrefix");

	set<string> kmerSetting;
	kmerSetting.insert("-k");

	set<string> routingDegree;
	routingDegree.insert("-routing-graph-degree");

	set<string> connectionType;
	connectionType.insert("-connection-type");

	set<string> reduceMemoryUsage;
	reduceMemoryUsage.insert("-r");

	set<string> showMemory;
	showMemory.insert("-show-memory-usage");
	showMemory.insert("--show-memory-usage");
	set<string> debugBubbles;
	debugBubbles.insert("-debug-bubbles");
	debugBubbles.insert("--debug-bubbles");

	set<string> debugSeeds;
	debugSeeds.insert("-debug-seeds");
	debugSeeds.insert("--debug-seeds");

	set<string> runProfiler;
	runProfiler.insert("-run-profiler");
	runProfiler.insert("--run-profiler");

	set<string> showContext;
	showContext.insert("-show-ending-context");
	showContext.insert("--show-ending-context");

	set<string> showExtensionChoiceOption;
	showExtensionChoiceOption.insert("-show-extension-choice");

	set<string> writeKmers;
	writeKmers.insert("-write-kmers");

	set<string> setMinimumCoverage;
	set<string> setPeakCoverage;
	set<string> setRepeatCoverage;
	setMinimumCoverage.insert("-minimumCoverage");
	setPeakCoverage.insert("-peakCoverage");
	setRepeatCoverage.insert("-repeatCoverage");

	set<string> minimumContigLength;
	minimumContigLength.insert("-minimum-contig-length");

	set<string> searchOption;
	searchOption.insert("-search");

	set<string> phylogeny;
	phylogeny.insert("-with-taxonomy");

	set<string> coloringOneColor;
	coloringOneColor.insert("-one-color-per-file");

	set<string> ontology;
	ontology.insert("-gene-ontology");

	set<string> checkpoints;
	checkpoints.insert("-read-write-checkpoints");
	checkpoints.insert("-write-checkpoints");
	checkpoints.insert("-read-checkpoints");


	set<string> maximumSeedCoverage;
	maximumSeedCoverage.insert("-use-maximum-seed-coverage");

	vector<set<string> > toAdd;
	toAdd.push_back(checkpoints);
	toAdd.push_back(coloringOneColor);
	toAdd.push_back(ontology);
	toAdd.push_back(phylogeny);
	toAdd.push_back(minimumContigLength);
	toAdd.push_back(showExtensionChoiceOption);
	toAdd.push_back(setRepeatCoverage);
	toAdd.push_back(setPeakCoverage);
	toAdd.push_back(setMinimumCoverage);
	toAdd.push_back(singleReadsCommands);
	toAdd.push_back(pairedReadsCommands);
	toAdd.push_back(outputAmosCommands);
	toAdd.push_back(outputFileCommands);
	toAdd.push_back(kmerSetting);
	toAdd.push_back(routingDegree);
	toAdd.push_back(interleavedCommands);
	toAdd.push_back(searchOption);
	toAdd.push_back(reduceMemoryUsage);
	toAdd.push_back(memoryMappedFileCommands);
	toAdd.push_back(connectionType);
	toAdd.push_back(showMemory);	
	toAdd.push_back(debugBubbles);
	toAdd.push_back(debugSeeds);
	toAdd.push_back(runProfiler);
	toAdd.push_back(showContext);
	toAdd.push_back(showMalloc);
	toAdd.push_back(writeKmers);
	toAdd.push_back(colorSpaceMode);
	toAdd.push_back(maximumSeedCoverage);

	for(int i=0;i<(int)toAdd.size();i++){
		for(set<string>::iterator j=toAdd[i].begin();j!=toAdd[i].end();j++){
			commands.insert(*j);
		}
	}

	m_numberOfLibraries=0;

	bool providedMemoryPrefix=false;

	for(int i=0;i<(int)m_commands.size();i++){
		string token=m_commands[i];
		if(singleReadsCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_singleEndReadsFile.push_back(token);

			fileNameHook(token);

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-s (single sequences)"<<endl;
				cout<<" Sequences: "<<token<<endl;
			}
		}else if(memoryMappedFileCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;
			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_memoryFilePrefix=token;
			providedMemoryPrefix=true;

		}else if(checkpoints.count(token)>0){
			i++;
			int items=m_commands.size()-i;
			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];

			if(m_hasCheckpointDirectory){
				cout<<"Warning: can not set already set checkpoint directory."<<endl;
				continue;
			}

			m_checkpointDirectory=token;
			m_hasCheckpointDirectory=true;

			cout<<"Rank "<<m_rank<<" checkpoint directory: "<<m_checkpointDirectory;

			if(m_rank==MASTER_RANK){

				if(!fileExists(m_checkpointDirectory.c_str())){
					createDirectory(m_checkpointDirectory.c_str());
				}
			}

		}else if(outputFileCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;
			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_prefix=token;
			if(!providedMemoryPrefix){
				m_memoryFilePrefix=m_prefix;
			}
		}else if(interleavedCommands.count(token)>0){
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0 && cmd[0]!='-'){
					items++;
				}else{
					break;
				}
				k++;
			}
			if(items!=1 && items!=3){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 or 3 items, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			i++;
			token=m_commands[i];

			string interleavedFile=token;
			int interleavedFileIndex=m_singleEndReadsFile.size();
			m_interleavedFiles.insert(interleavedFileIndex);
			m_singleEndReadsFile.push_back(interleavedFile);

			fileNameHook(interleavedFile);

			int meanFragmentLength=0;
			int standardDeviation=0;
			#ifdef ASSERT
			assert(items==1 or items==3);
			#endif

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"Paired library # "<<m_numberOfLibraries<<endl;
				cout<<" -i (paired-end interleaved sequences)"<<endl;
				cout<<" Sequences: "<<token<<endl;
			}
			if(items==3){
				i++;
				token=m_commands[i];
				meanFragmentLength=atoi(token.c_str());
				i++;
				token=m_commands[i];
				standardDeviation=atoi(token.c_str());
				if(m_rank==MASTER_RANK){
					cout<<" Average length: "<<meanFragmentLength<<endl;
					cout<<" Standard deviation: "<<standardDeviation<<endl;
				}
				int distance=meanFragmentLength+standardDeviation;
				if(distance>m_maximumDistance){
					m_maximumDistance=distance;
				}
			}else if(items==1){// automatic detection.
				map<int,int> t;
				m_automaticLibraries.insert(m_numberOfLibraries);
				if(m_rank==MASTER_RANK){
					cout<<" Average length: automatic detection"<<endl;
					cout<<" Standard deviation: automatic detection"<<endl;
				}
			}else{
				#ifdef ASSERT
				assert(false);
				#endif
			}

			m_fileLibrary[interleavedFileIndex]=m_numberOfLibraries;
			vector<int> files;
			files.push_back(interleavedFileIndex);
			m_libraryFiles.push_back(files);

			addLibraryData(m_numberOfLibraries,meanFragmentLength,standardDeviation);

			m_numberOfLibraries++;

		}else if(maximumSeedCoverage.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];

			m_maximumSeedCoverage=atoi(token.c_str());

			cout<<"[Parameters] changed the maximum seed coverage depth to ";
			cout<<m_maximumSeedCoverage<<endl;

		}else if(pairedReadsCommands.count(token)>0){
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0 && cmd[0]!='-'){
					items++;
				}else{
					break;
				}
				k++;
			}
			if(items!=2 && items!=4){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 2 or 4 items, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			i++;
			token=m_commands[i];
			string left=token;
			// add left file
			int leftFile=m_singleEndReadsFile.size();
			m_leftFiles.insert(leftFile);
			m_singleEndReadsFile.push_back(left);
			i++;
			token=m_commands[i];
			
			// add right file
			string right=token;
			int rightFile=m_singleEndReadsFile.size();
			m_rightFiles.insert(rightFile);
			m_singleEndReadsFile.push_back(right);

			fileNameHook(left);
			fileNameHook(right);

			int meanFragmentLength=0;
			int standardDeviation=0;
			#ifdef ASSERT
			assert(items==4 or items==2);
			#endif

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"Paired library # "<<m_numberOfLibraries<<endl;
				cout<<" -p (paired-end sequences)"<<endl;
				cout<<" Left sequences: "<<left<<endl;
				cout<<" Right sequences: "<<right<<endl;
			}

			bool gotValuesFromEndUser=false;

/* try to use what the user provided */

			if(items==4){

				if(!isValidInteger(m_commands[i+1].c_str())
					|| !isValidInteger(m_commands[i+2].c_str())){
					
					if(m_rank==MASTER_RANK){
						cout<<"Warning: invalid integer values for distances, you provided: -p ";
						cout<<left<<" "<<right<<" "<<m_commands[i+1];
						cout<<" "<<m_commands[i+2];
						cout<<endl;
					}

					i+=2; /* consume the data */
				}else{
					i++;
					token=m_commands[i];
					meanFragmentLength=atoi(token.c_str());
					i++;
					token=m_commands[i];
					standardDeviation=atoi(token.c_str());
					if(m_rank==MASTER_RANK){
						cout<<" Average length: "<<meanFragmentLength<<endl;
						cout<<" Standard deviation: "<<standardDeviation<<endl;
					}

					gotValuesFromEndUser=true;
				}
			}

			if(!gotValuesFromEndUser){// automatic detection.
				m_automaticLibraries.insert(m_numberOfLibraries);
				if(m_rank==MASTER_RANK){
					cout<<" Average length: automatic detection"<<endl;
					cout<<" Standard deviation: automatic detection"<<endl;
				}
			}

			m_fileLibrary[rightFile]=m_numberOfLibraries;
			m_fileLibrary[leftFile]=m_numberOfLibraries;
			vector<int> files;
			files.push_back(leftFile);
			files.push_back(rightFile);
			m_libraryFiles.push_back(files);

			addLibraryData(m_numberOfLibraries,meanFragmentLength,standardDeviation);

			m_numberOfLibraries++;
		}else if(outputAmosCommands.count(token)>0){
			m_amos=true;
		}else if(showExtensionChoiceOption.count(token)>0){
			m_showExtensionChoice=true;
		}else if(showMalloc.count(token)>0){
			m_showMemoryAllocations=true;
		}else if(reduceMemoryUsage.count(token)>0){
			int items=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
					items++;
				}else{
					break;
				}
			}

			if(!(items==0||items==1)){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 0 or 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}

			m_reducerIsActivated=true;
			m_reducerPeriod=1000000;
			
			if(items==1){
				m_reducerPeriod=atoi(m_commands[i+1].c_str());
			}
		}else if(setRepeatCoverage.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_repeatCoverage=atoi(token.c_str());
			m_providedRepeatCoverage=true;
		}else if(setMinimumCoverage.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_minimumCoverage=atoi(token.c_str());
			m_providedMinimumCoverage=true;
		}else if(setPeakCoverage.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_peakCoverage=atoi(token.c_str());
			m_providedPeakCoverage=true;
	
		}else if(connectionType.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item but you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_connectionType=token;

		}else if(routingDegree.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_degree=atoi(token.c_str());

		}else if(phylogeny.count(token)>0){

			cout<<"Enabling CorePlugin 'PhylogenyViewer'"<<endl;

			i++;
			int items=m_commands.size()-i;

			if(items<3){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 3 item, but you provided "<<items<<endl;
				}

				m_error=true;
				return;
			}

			m_genomeToTaxonFile=m_commands[i];
			i++;
			m_treeFile=m_commands[i];
			i++;
			m_taxonNameFile=m_commands[i];

		}else if(searchOption.count(token)>0){

			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];

			m_searchDirectories.push_back(token);

		}else if(minimumContigLength.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_minimumContigLength=atoi(token.c_str());

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"Rank "<<MASTER_RANK<<" the minimum contig length is "<<m_minimumContigLength<<endl;
				cout<<endl;
			}

		}else if(kmerSetting.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_wordSize=atoi(token.c_str());
			if(m_wordSize<15){
				m_wordSize=15;
			}
			if(m_wordSize>MAXKMERLENGTH){
				if(m_rank==MASTER_RANK){
					cout<<endl;
					cout<<"Rank "<<MASTER_RANK<<": Warning, k > MAXKMERLENGTH"<<endl;
					cout<<"Rank "<<MASTER_RANK<<": Change MAXKMERLENGTH in the Makefile and recompile Ray."<<endl;
				}
				m_wordSize=MAXKMERLENGTH;
			}

			if(m_wordSize%2==0){
				m_wordSize--;
			}
			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-k (to set the k-mer size)"<<endl;
				cout<<" Value: "<<m_wordSize<<endl;
				cout<<endl;
			}
		}else if(writeKmers.count(token)>0){
			m_writeKmers=true;
			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"Ray will write k-mers ("<<token<<")"<<endl;

			}
		}else if(runProfiler.count(token)>0){
			m_profiler=true;
			if(m_rank==MASTER_RANK){
				printf("Enabling profiler!\n");
			}
		}else if(debugBubbles.count(token)>0){
			m_debugBubbles=true;
			if(m_rank==MASTER_RANK){
				printf("Enabling bubble debug mode.\n");
			}
		}else if(colorSpaceMode.count(token)>0){
			m_colorSpaceMode=true;
			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"Enabling color-space mode"<<endl;
				cout<<"All reads should be in color space."<<endl;
			}
		}else if(debugSeeds.count(token)>0){
			m_debugSeeds=true;
			if(m_rank==MASTER_RANK){
				printf("Enabling seed debug mode.\n");
			}
		}else if(showMemory.count(token)>0){
			m_showMemoryUsage=true;
			if(m_rank==MASTER_RANK){
				printf("Enabling memory usage reporting.\n");
			}
		}else if(showContext.count(token)>0){
			m_showEndingContext=true;
			if(m_rank==MASTER_RANK){
				printf("Ray will show the ending context of extensions.\n");
			}
		}
	}

	int maximumNumberOfFiles=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint32_t);
	
	assert((int)m_singleEndReadsFile.size()<=maximumNumberOfFiles);

	LargeCount result=1;
	for(int p=0;p<m_wordSize;p++){
		result*=4;
	}
}

void Parameters::writeCommandFile(){
	cout<<endl;
	cout<<"Ray command: "<<endl;

	ostringstream commandFile;
	commandFile<<getPrefix()<<"RayCommand.txt";
	ofstream f(commandFile.str().c_str());

	f<<"mpiexec -n "<<getSize()<<" Ray \\"<<endl;
	cout<<"mpiexec -n "<<getSize()<<" Ray \\"<<endl;

	for(int i=0;i<(int)m_originalCommands.size();i++){
		if(i!=(int)m_originalCommands.size()-1){
			f<<" "<<m_originalCommands[i]<<" \\"<<endl;
			cout<<" "<<m_originalCommands[i]<<" \\"<<endl;
		}else{
			f<<" "<<m_originalCommands[i]<<endl;
			cout<<" "<<m_originalCommands[i]<<endl;
		}
	}
	f.close();
	cout<<endl;

	cout<<"Rank "<<MASTER_RANK<<" wrote "<<commandFile.str()<<endl;
	cout<<endl;

	cout<<"k-mer length: "<<m_wordSize<<endl;
	
	if(m_reducerIsActivated){
		cout<<"Memory Consumption Reducer is enabled, threshold="<<m_reducerPeriod<<endl;
	}
	cout<<endl;
	cout<<"Output files will be prefixed with "<<getPrefix()<<endl;
	cout<<endl;

	ostringstream rayRuntime;
	rayRuntime<<getPrefix()<<"RayVersion.txt";
	ofstream f2(rayRuntime.str().c_str());
	f2<<"Ray version: "<<RAY_VERSION<<endl;
	f2.close();

	writeConfigurationFile();
}

void Parameters::writeConfigurationFile(){
	if(m_configurationContent=="")
		return;

	ostringstream file;
	file<<getPrefix()<<"/Ray.conf";
	ofstream f(file.str().c_str());
	f<<m_configurationContent;
	f.close();
}

void Parameters::constructor(int argc,char**argv,int rank,int size){
	m_maximumDistance=0;
	m_totalNumberOfSequences=0;

	m_rank=rank;
	m_size=size;

	bool hasCommandFile=false;
	if(argc==2){
		ifstream f(argv[1]);
		hasCommandFile=f;
		f.close();
	}

	if(argc==2&&hasCommandFile){
		m_input=argv[1];
		loadCommandsFromFile(argv[1]);
	}else{
		loadCommandsFromArguments(argc,argv);
	}
	parseCommands();
}

int Parameters::getRank(){
	return m_rank;
}

bool Parameters::isInitiated(){
	return m_initiated;
}

vector<string> Parameters::getAllFiles(){
	vector<string> l;
	for(int i=0;i<(int)m_singleEndReadsFile.size();i++)
		l.push_back(m_singleEndReadsFile[i]);
	return l;
}

string Parameters::getFile(int file){
	return m_singleEndReadsFile[file];
}

string Parameters::getDirectory(){
	return m_directory;
}

string Parameters::getOutputFile(){
	return getPrefix()+"Contigs.fasta";
}

int Parameters::getMinimumContigLength(){
	return m_minimumContigLength;
}

bool Parameters::isLeftFile(int i){
	return m_leftFiles.count(i)>0;
}

bool Parameters::isRightFile(int i){
	return m_rightFiles.count(i)>0;
}

int Parameters::getLibraryAverageLength(int i,int j){
	return m_libraryAverageLength[i][j];
}

int Parameters::getLibraryStandardDeviation(int i,int j){
	return m_libraryDeviation[i][j];
}

int Parameters::getLibraryMaxAverageLength(int i){
	if(m_libraryAverageLength[i].size()==1)
		return m_libraryAverageLength[i][0];

	int max=0;
	for(int j=0;j<(int)m_libraryAverageLength[i].size();j++)
		if(m_libraryAverageLength[i][j]>max)
			max=m_libraryAverageLength[i][j];
	return max;
}

int Parameters::getLibraryMaxStandardDeviation(int i){
	if(m_libraryDeviation[i].size()==1)
		return m_libraryDeviation[i][0];

	int max=0;
	for(int j=0;j<(int)m_libraryDeviation[i].size();j++)
		if(m_libraryDeviation[i][j]>max)
			max=m_libraryDeviation[i][j];
	return max;

}

bool Parameters::getColorSpaceMode(){
	return m_colorSpaceMode;
}

bool Parameters::useAmos(){
	return m_amos;
}

string Parameters::getInputFile(){
	return m_input;
}

string Parameters::getParametersFile(){
	return "Ray-Parameters.txt";
}

string Parameters::getPrefix(){
	ostringstream directory;
	directory<<m_prefix<<"/";
	return directory.str();
}

string Parameters::getCoverageDistributionFile(){
	return getPrefix()+"CoverageDistribution.txt";
}

string Parameters::getAmosFile(){
	return getPrefix()+"AMOS.afg";
}

vector<string> Parameters::getCommands(){
	return m_commands;
}

bool Parameters::getError(){
	return m_error;
}

void Parameters::addDistance(int library,int distance,int count){
	m_observedDistances[library][distance]+=count;
}

string Parameters::getLibraryFile(int library){
	ostringstream s;
	s<<getPrefix();
	s<<""<<"Library"<<library<<".txt";
	return s.str();
}

#define WRITE_LIBRARY_OBSERVATIONS

void Parameters::computeAverageDistances(){
	cout<<endl;
	for(map<int,map<int,int> >::iterator i=m_observedDistances.begin();
		i!=m_observedDistances.end();i++){
		int library=i->first;

		if(!isAutomatic(library))
			continue;

		vector<int> x;
		vector<int> y;
		string fileName=getLibraryFile(library);
		#ifdef WRITE_LIBRARY_OBSERVATIONS
		ofstream f(fileName.c_str());
		#endif
		for(map<int,int>::iterator j=m_observedDistances[library].begin();
			j!=m_observedDistances[library].end();j++){
			int d=j->first;
			int count=j->second;
			#ifdef WRITE_LIBRARY_OBSERVATIONS
			f<<d<<"\t"<<count<<endl;
			#endif
			x.push_back(d);
			y.push_back(count);
		}
		#ifdef WRITE_LIBRARY_OBSERVATIONS
		f.close();
		#endif

		vector<int> averages;
		vector<int> deviations;
		LibraryPeakFinder finder;
		finder.findPeaks(&x,&y,&averages,&deviations);
	
		for(int i=0;i<(int)averages.size();i++)
			addLibraryData(library,averages[i],deviations[i]);

	}	
	cout<<endl;
	cout<<endl;

	ostringstream fileName;
	fileName<<getPrefix();
	fileName<<"LibraryStatistics.txt";
	ofstream f2(fileName.str().c_str());

	f2<<"NumberOfPairedLibraries: "<<m_numberOfLibraries<<endl;
	f2<<endl;
	for(int i=0;i<(int)m_numberOfLibraries;i++){
		int library=i;
		string type="Manual";
		if(m_automaticLibraries.count(library)>0){
			type="Automatic";
		}
		f2<<"LibraryNumber: "<<library<<endl;
		string format="Interleaved,Paired";
		vector<int> files=m_libraryFiles[i];
		if(files.size()==2){
			format="TwoFiles,Paired";
		}
		f2<<" InputFormat: "<<format<<endl;
		f2<<" DetectionType: "<<type<<endl;
		f2<<" File: "<<m_singleEndReadsFile[files[0]]<<endl;
		f2<<"  NumberOfSequences: "<<m_numberOfSequencesInFile[files[0]]<<endl;
		if(files.size()>1){
			f2<<" File: "<<m_singleEndReadsFile[files[1]]<<endl;
			f2<<"  NumberOfSequences: "<<m_numberOfSequencesInFile[files[1]]<<endl;
		}
		f2<<" Distribution: "<<getLibraryFile(library)<<endl;
		for(int j=0;j<getLibraryPeaks(library);j++){
			int average=getLibraryAverageLength(library,j);
			int standardDeviation=getLibraryStandardDeviation(library,j);
			cout<<"Library # "<<library<<" ("<<type<<") -> average length: "<<average<<" and standard deviation: "<<standardDeviation<<endl;
			f2<<" Peak "<<j<<endl;
			f2<<"  AverageOuterDistance: "<<average<<endl;
			f2<<"  StandardDeviation: "<<standardDeviation<<endl;
			if(standardDeviation*2>average){
				f2<<"  DetectionFailure: Yes"<<endl;
			}
		}
		f2<<endl;
	}
	f2.close();
}

void Parameters::addLibraryData(int library,int average,int deviation){
	if(average==0)
		return;

	m_libraryAverageLength[library].push_back(average);
	m_libraryDeviation[library].push_back(deviation);
	
	int distance=average+4*deviation;
	if(distance>m_maximumDistance){
		m_maximumDistance=distance;
	}
}

void Parameters::setNumberOfSequences(int file,LargeCount n){
	if(m_numberOfSequencesInFile.count(file)==0){
		m_numberOfSequencesInFile[file]=n;
		m_totalNumberOfSequences+=n;
	}
}

int Parameters::getNumberOfLibraries(){
	return m_numberOfLibraries;
}

LargeCount Parameters::getNumberOfSequences(int file){
	#ifdef ASSERT
	if(file>=(int)m_numberOfSequencesInFile.size())
		cout<<"Error File= "<<file<<" Files: "<<m_numberOfSequencesInFile.size()<<endl;

	assert(file<(int)m_numberOfSequencesInFile.size());
	#endif
	return m_numberOfSequencesInFile[file];
}

int Parameters::getNumberOfFiles(){
	return m_singleEndReadsFile.size();
}

bool Parameters::isAutomatic(int library){
	return m_automaticLibraries.count(library)>0;
}

int Parameters::getLibrary(int file){
	return m_fileLibrary[file];
}

bool Parameters::isInterleavedFile(int i){
	return m_interleavedFiles.count(i)>0;
}

bool Parameters::showMemoryUsage(){
	return m_showMemoryUsage;
}

string Parameters::getReceivedMessagesFile(){
	string outputForMessages=getPrefix()+"ReceivedMessages.txt";
	return outputForMessages;
}

void Parameters::printFinalMessage(){
	cout<<"Rank "<<MASTER_RANK<<" wrote library statistics"<<endl;
}

CoverageDepth Parameters::getMaximumAllowedCoverage(){
	CoverageDepth a=0;
	a--;
	return a;
}

void Parameters::setPeakCoverage(CoverageDepth a){
	if(!m_providedPeakCoverage){
		m_peakCoverage=a;
	}
}

void Parameters::setRepeatCoverage(CoverageDepth a){
	if(!m_providedRepeatCoverage){
		m_repeatCoverage=a;
	}
}

CoverageDepth Parameters::getPeakCoverage(){
	return m_peakCoverage;
}

CoverageDepth Parameters::getRepeatCoverage(){
	return m_repeatCoverage;
}

int Parameters::getSize(){
	return m_size;
}

void Parameters::setSize(int a){
	m_size=a;
}

bool Parameters::runReducer(){
	return m_reducerIsActivated;
}

void Parameters::showOption(string option,string description){
	string spacesBeforeOption="       ";
	cout<<spacesBeforeOption<<option<<endl;
	showOptionDescription(description);
}

void Parameters::showOptionDescription(string description){
	string spacesBeforeDescription="              ";
	cout<<spacesBeforeDescription<<description<<endl;
}

/* obviously shows usage */
void Parameters::showUsage(){
	string basicSpaces="       ";
	cout<<"NAME"<<endl<<basicSpaces<<"Ray - assemble genomes in parallel using the message-passing interface"<<endl<<endl;

	cout<<"SYNOPSIS"<<endl;
	cout<<basicSpaces<<"mpiexec -n NUMBER_OF_RANKS Ray -k KMERLENGTH -p l1_1.fastq l1_2.fastq -p l2_1.fastq l2_2.fastq -o test"<<endl;
	cout<<endl;
	cout<<basicSpaces<<"mpiexec -n NUMBER_OF_RANKS Ray Ray.conf # with commands in a file"<<endl;
	cout<<endl;

	cout<<"DESCRIPTION:"<<endl;

	cout<<endl;
	cout<<"  The Ray genome assembler is built on top of the RayPlatform, a generic plugin-based"<<endl;
	cout<<"  distributed and parallel compute engine that uses the message-passing interface"<<endl;
	cout<<"  for passing messages."<<endl;
	cout<<endl;
	cout<<"  Ray targets several applications:"<<endl;
	cout<<endl;
	cout<<"    - de novo genome assembly (with Ray vanilla)"<<endl;
	cout<<"    - de novo meta-genome assembly (with Ray Méta)"<<endl;
	cout<<"    - de novo transcriptome assembly (works, but not tested a lot)"<<endl;
	cout<<"    - quantification of contig abundances"<<endl;
	cout<<"    - quantification of microbiome consortia members (with Ray Communities)"<<endl;
	cout<<"    - quantification of transcript expression"<<endl;
	cout<<"    - taxonomy profiling of samples (with Ray Communities)"<<endl;
	cout<<"    - gene ontology profiling of samples (with Ray Ontologies)"<<endl;
	cout<<endl;
	showOption("-help","Displays this help page.");
	cout<<endl;
	showOption("-version","Displays Ray version and compilation options.");
	cout<<endl;

	cout<<"  Using a configuration file"<<endl;
	cout<<endl;
	cout<<"    Ray can be launched with"<<endl;
	cout<<"    mpiexec -n 16 Ray Ray.conf"<<endl;
	cout<<"    The configuration file can include comments (starting with #)."<<endl;

	cout<<endl;
	cout<<"  K-mer length"<<endl;
	cout<<endl;
	showOption("-k kmerLength","Selects the length of k-mers. The default value is 21. ");
	showOptionDescription("It must be odd because reverse-complement vertices are stored together.");
	showOptionDescription("The maximum length is defined at compilation by MAXKMERLENGTH");
	showOptionDescription("Larger k-mers utilise more memory.");
	cout<<endl;


	cout<<"  Inputs"<<endl;
	cout<<endl;
	showOption("-p leftSequenceFile rightSequenceFile [averageOuterDistance standardDeviation]","Provides two files containing paired-end reads.");
	showOptionDescription("averageOuterDistance and standardDeviation are automatically computed if not provided.");
	cout<<endl;
	showOption("-i interleavedSequenceFile [averageOuterDistance standardDeviation]","Provides one file containing interleaved paired-end reads.");

	showOptionDescription("averageOuterDistance and standardDeviation are automatically computed if not provided.");
	cout<<endl;

	showOption("-s sequenceFile","Provides a file containing single-end reads.");
	cout<<endl;

	cout<<"  Outputs"<<endl;
	cout<<endl;
	showOption("-o outputDirectory","Specifies the directory for outputted files. Default is RayOutput");
	cout<<endl;


	cout<<"  Assembly options (defaults work well)"<<endl;
	cout<<endl;
	
	showOption("-disable-recycling","Disables read recycling during the assembly");
	showOptionDescription("reads will be set free in 3 cases:");
	showOptionDescription("1. the distance did not match for a pair");
	showOptionDescription("2. the read has not met its mate");
	showOptionDescription("3. the library population indicates a wrong placement");
	showOptionDescription("see Constrained traversal of repeats with paired sequences.");
	showOptionDescription("Sébastien Boisvert, Élénie Godzaridis, François Laviolette & Jacques Corbeil.");
	showOptionDescription("First Annual RECOMB Satellite Workshop on Massively Parallel Sequencing, March 26-27 2011, Vancouver, BC, Canada.");
	cout<<endl;

	showOption("-disable-scaffolder","Disables the scaffolder.");
	cout<<endl;
	showOption("-minimum-contig-length minimumContigLength",
		"Changes the minimum contig length, default is 100 nucleotides");
	cout<<endl;

	showOption("-color-space","Runs in color-space");
	showOptionDescription("Needs csfasta files. Activated automatically if csfasta files are provided.");
	cout<<endl;

	ostringstream buffer3;
	buffer3<<"The default is "<<getMaximumAllowedCoverage()<<".";
	showOption("-use-maximum-seed-coverage maximumSeedCoverageDepth",
		"Ignores any seed with a coverage depth above this threshold.");
	showOptionDescription(buffer3.str());
	cout<<endl;

	showOption("-use-minimum-seed-coverage minimumSeedCoverageDepth",
		"Sets the minimum seed coverage depth.");
	showOptionDescription("Any path with a coverage depth lower than this will be discarded. The default is 0.");
	cout<<endl;


	cout<<"  Distributed storage engine (all these values are for each MPI rank)"<<endl;
	cout<<endl;

	ostringstream text;
	showOption("-bloom-filter-bits bits","Sets the number of bits for the Bloom filter");
	text<<"Default is "<<__BLOOM_DEFAULT_BITS<<" bits, 0 bits disables the Bloom filter.";
	showOptionDescription(text.str());
	cout<<endl;
	
	text.str("");
	text<<"Default value: "<<__DEFAULT_BUCKETS;
	showOption("-hash-table-buckets buckets","Sets the initial number of buckets. Must be a power of 2 !");
	showOptionDescription(text.str());
	text.str("");

	cout<<endl;
	text<<"Default value: "<<__DEFAULT_BUCKETS_PER_GROUP<<", Must be between >=1 and <= 64";
	showOption("-hash-table-buckets-per-group buckets",
		"Sets the number of buckets per group for sparse storage");
	showOptionDescription(text.str());
	text.str("");
	cout<<endl;

	showOption("-hash-table-load-factor-threshold threshold","Sets the load factor threshold for real-time resizing");

	text<<"Default value: "<< __DEFAULT_LOAD_FACTOR_THRESHOLD<<", must be >= 0.5 and < 1";
	showOptionDescription(text.str());
	cout<<endl;
	
	showOption("-hash-table-verbosity","Activates verbosity for the distributed storage engine");
	cout<<endl;

	cout<<"  Biological abundances"<<endl;
	cout<<endl;
	showOption("-search searchDirectory","Provides a directory containing fasta files to be searched in the de Bruijn graph.");
	showOptionDescription("Biological abundances will be written to RayOutput/BiologicalAbundances");
	showOptionDescription("See Documentation/BiologicalAbundances.txt");
	cout<<endl;

	showOption("-one-color-per-file", "Sets one color per file instead of one per sequence.");
	showOptionDescription("By default, each sequence in each file has a different color.");
	showOptionDescription("For files with large numbers of sequences, using one single color per file may be more efficient.");
	cout<<endl;

	cout<<"  Taxonomic profiling with colored de Bruijn graphs"<<endl;
	cout<<endl;
	showOption("-with-taxonomy Genome-to-Taxon.tsv TreeOfLife-Edges.tsv Taxon-Names.tsv","Provides a taxonomy.");
	showOptionDescription("Computes and writes detailed taxonomic profiles.");
	showOptionDescription("See Documentation/Taxonomy.txt for details.");

	cout<<endl;
	showOption("-gene-ontology OntologyTerms.txt  Annotations.txt","Provides an ontology and annotations.");
	showOptionDescription("OntologyTerms.txt is fetched from http://geneontology.org");
	showOptionDescription("Annotations.txt is a 2-column file (EMBL_CDS handle	&	gene ontology identifier)");
	showOptionDescription("See Documentation/GeneOntology.txt");
	

	cout<<"  Other outputs"<<endl;
	cout<<endl;
	showOption("-enable-neighbourhoods","Computes contig neighborhoods in the de Bruijn graph");
	showOptionDescription("Output file: RayOutput/NeighbourhoodRelations.txt");
	cout<<endl;

	showOption("-amos","Writes the AMOS file called RayOutput/AMOS.afg");
	showOptionDescription("An AMOS file contains read positions on contigs.");
	showOptionDescription("Can be opened with software with graphical user interface.");
	cout<<endl;
	showOption("-write-kmers","Writes k-mer graph to RayOutput/kmers.txt");
	showOptionDescription("The resulting file is not utilised by Ray.");
	showOptionDescription("The resulting file is very large.");
	cout<<endl;
	showOption("-write-read-markers","Writes read markers to disk.");
	cout<<endl;
	showOption("-write-seeds","Writes seed DNA sequences to RayOutput/Rank<rank>.RaySeeds.fasta");
	cout<<endl;
	showOption("-write-extensions","Writes extension DNA sequences to RayOutput/Rank<rank>.RayExtensions.fasta");
	cout<<endl;
	showOption("-write-contig-paths","Writes contig paths with coverage values");
	showOptionDescription("to RayOutput/Rank<rank>.RayContigPaths.txt");
	cout<<endl;
	showOption("-write-marker-summary","Writes marker statistics.");
	cout<<endl;

	cout<<"  Memory usage"<<endl;
	cout<<endl;
	showOption("-show-memory-usage","Shows memory usage. Data is fetched from /proc on GNU/Linux");
	showOptionDescription("Needs __linux__");
	cout<<endl;
	showOption("-show-memory-allocations","Shows memory allocation events");
	cout<<endl;

	cout<<"  Algorithm verbosity"<<endl;
	cout<<endl;
	showOption("-show-extension-choice","Shows the choice made (with other choices) during the extension.");
	cout<<endl;
	showOption("-show-ending-context","Shows the ending context of each extension.");
	showOptionDescription("Shows the children of the vertex where extension was too difficult.");
	cout<<endl;
	showOption("-show-distance-summary","Shows summary of outer distances used for an extension path.");
	cout<<endl;
	showOption("-show-consensus","Shows the consensus when a choice is done.");
	cout<<endl;

/*
	showOption("-minimumCoverage minimumCoverage","Sets manually the minimum coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
	showOption("-peakCoverage peakCoverage","Sets manually the peak coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
	showOption("-repeatCoverage repeatCoverage","Sets manually the repeat coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
*/

	cout<<"  Checkpointing"<<endl;
	cout<<endl;

	showOption("-write-checkpoints checkpointDirectory","Write checkpoint files");
	cout<<endl;
	showOption("-read-checkpoints checkpointDirectory","Read checkpoint files");
	cout<<endl;
	showOption("-read-write-checkpoints checkpointDirectory","Read and write checkpoint files");
	cout<<endl;

	cout<<"  Message routing for large number of cores"<<endl;
	cout<<endl;
	showOption("-route-messages","Enables the Ray message router. Disabled by default.");
	showOptionDescription("Messages will be routed accordingly so that any rank can communicate directly with only a few others.");
	showOptionDescription("Without -route-messages, any rank can communicate directly with any other rank.");
	showOptionDescription("Files generated: Routing/Connections.txt, Routing/Routes.txt and Routing/RelayEvents.txt");
	showOptionDescription("and Routing/Summary.txt");
	cout<<endl;
	showOption("-connection-type type","Sets the connection type for routes.");
	showOptionDescription("Accepted values are debruijn, hypercube, polytope, group, random, kautz and complete. Default is debruijn.");
	showOptionDescription(" debruijn: a full de Bruijn graph a given alphabet and diameter");
	showOptionDescription(" hypercube: a hypercube, alphabet is {0,1} and the vertices is a power of 2");
	showOptionDescription(" polytope: a convex regular polytope, alphabet is {0,1,...,B-1} and the vertices is a power of B");
	showOptionDescription(" group: silly model where one representative per group can communicate with outsiders");
	showOptionDescription(" random: Erdős–Rényi model");
	showOptionDescription(" kautz: a full de Kautz graph, which is a subgraph of a de Bruijn graph");
	showOptionDescription(" complete: a full graph with all the possible connections");

	showOptionDescription("With the type debruijn, the number of ranks must be a power of something.");
	showOptionDescription("Examples: 256 = 16*16, 512=8*8*8, 49=7*7, and so on.");
	showOptionDescription("Otherwise, don't use debruijn routing but use another one");
	showOptionDescription("With the type kautz, the number of ranks n must be n=(k+1)*k^(d-1) for some k and d");
	cout<<endl;

	showOption("-routing-graph-degree degree","Specifies the outgoing degree for the routing graph.");
	showOptionDescription("See Documentation/Routing.txt");
	cout<<endl;

	cout<<"  Hardware testing"<<endl;
	cout<<endl;
	showOption("-test-network-only","Tests the network and returns.");
	cout<<endl;
	showOption("-write-network-test-raw-data","Writes one additional file per rank detailing the network test.");
	cout<<endl;

	showOption("-exchanges NumberOfExchanges","Sets the number of exchanges");
	cout<<endl;

	showOption("-disable-network-test","Skips the network test.");
	cout<<endl;

	cout<<"  Debugging"<<endl;
	cout<<endl;

	showOption("-verify-message-integrity","Checks message data reliability for any non-empty message.");
	showOptionDescription("add '-D CONFIG_SSE_4_2' in the Makefile to use hardware instruction (SSE 4.2)");
	cout<<endl;


	showOption("-run-profiler","Runs the profiler as the code runs. By default, only show granularity warnings.");
	showOptionDescription("Running the profiler increases running times.");
	cout<<endl;
	showOption("-with-profiler-details","Shows number of messages sent and received in each methods during in each time slices (epochs). Needs -run-profiler.");
	cout<<endl;
	showOption("-show-communication-events","Shows all messages sent and received.");
	cout<<endl;
	showOption("-show-read-placement","Shows read placement in the graph during the extension.");
	cout<<endl;
	showOption("-debug-bubbles","Debugs bubble code.");
	showOptionDescription("Bubbles can be due to heterozygous sites or sequencing errors or other (unknown) events");
	cout<<endl;
	showOption("-debug-seeds","Debugs seed code.");
	showOptionDescription("Seeds are paths in the graph that are likely unique.");
	cout<<endl;
	showOption("-debug-fusions","Debugs fusion code.");
	cout<<endl;
	showOption("-debug-scaffolder","Debug the scaffolder.");
	cout<<endl;
	cout<<endl;


	cout<<"FILES"<<endl;
	cout<<endl;
	cout<<"  Input files"<<endl;
	cout<<endl;
	cout<<"     Note: file format is determined with file extension."<<endl;
	cout<<endl;
	cout<<"     .fasta"<<endl;
	cout<<"     .fasta.gz (needs HAVE_LIBZ=y at compilation)"<<endl;
	cout<<"     .fasta.bz2 (needs HAVE_LIBBZ2=y at compilation)"<<endl;
	cout<<"     .fastq"<<endl;
	cout<<"     .fastq.gz (needs HAVE_LIBZ=y at compilation)"<<endl;
	cout<<"     .fastq.bz2 (needs HAVE_LIBBZ2=y at compilation)"<<endl;
	cout<<"     .sff (paired reads must be extracted manually)"<<endl;
	cout<<"     .csfasta (color-space reads)"<<endl;


	cout<<endl;
	cout<<"  Outputted files"<<endl;
	cout<<endl;

	cout<<"  Scaffolds"<<endl;
	cout<<endl;
	cout<<"     RayOutput/Scaffolds.fasta"<<endl;
	cout<<"     	The scaffold sequences in FASTA format"<<endl;
	cout<<"     RayOutput/ScaffoldComponents.txt"<<endl;
	cout<<"     	The components of each scaffold"<<endl;
	cout<<"     RayOutput/ScaffoldLengths.txt"<<endl;
	cout<<"     	The length of each scaffold"<<endl;
	cout<<"     RayOutput/ScaffoldLinks.txt"<<endl;
	cout<<"     	Scaffold links"<<endl;
	cout<<endl;

	cout<<"  Contigs"<<endl;
	cout<<endl;
	cout<<"     RayOutput/Contigs.fasta"<<endl;
	cout<<"     	Contiguous sequences in FASTA format"<<endl;
	cout<<"     RayOutput/ContigLengths.txt"<<endl;
	cout<<"     	The lengths of contiguous sequences"<<endl;
	cout<<endl;

	cout<<"  Summary"<<endl;
	cout<<endl;
	cout<<"     RayOutput/OutputNumbers.txt"<<endl;
	cout<<"     	Overall numbers for the assembly"<<endl;
	cout<<endl;

	cout<<"  de Bruijn graph"<<endl;
	cout<<endl;
	cout<<"     RayOutput/CoverageDistribution.txt"<<endl;
	cout<<"     	The distribution of coverage values"<<endl;
	cout<<"     RayOutput/CoverageDistributionAnalysis.txt"<<endl;
	cout<<"     	Analysis of the coverage distribution"<<endl;
	cout<<"     RayOutput/degreeDistribution.txt"<<endl;
	cout<<"     	Distribution of ingoing and outgoing degrees"<<endl;
	cout<<"     RayOutput/kmers.txt"<<endl;
	cout<<"     	k-mer graph, required option: -write-kmers"<<endl;
	cout<<"         The resulting file is not utilised by Ray."<<endl;
	cout<<"         The resulting file is very large."<<endl;
	cout<<endl;
	
	cout<<"  Assembly steps"<<endl;
	cout<<endl;
	cout<<"     RayOutput/SeedLengthDistribution.txt"<<endl;
	cout<<"         Distribution of seed length"<<endl;
	cout<<"     RayOutput/Rank<rank>.OptimalReadMarkers.txt"<<endl;
	cout<<"         Read markers."<<endl;
	cout<<"     RayOutput/Rank<rank>.RaySeeds.fasta"<<endl;
	cout<<"         Seed DNA sequences, required option: -write-seeds"<<endl;
	cout<<"     RayOutput/Rank<rank>.RayExtensions.fasta"<<endl;
	cout<<"         Extension DNA sequences, required option: -write-extensions"<<endl;
	cout<<"     RayOutput/Rank<rank>.RayContigPaths.txt"<<endl;
	cout<<"         Contig paths with coverage values, required option: -write-contig-paths"<<endl;
	cout<<endl;

	cout<<"  Paired reads"<<endl;
	cout<<endl;
	cout<<"     RayOutput/LibraryStatistics.txt"<<endl;
	cout<<"     	Estimation of outer distances for paired reads"<<endl;
	cout<<"     RayOutput/Library<LibraryNumber>.txt"<<endl;
	cout<<"         Frequencies for observed outer distances (insert size + read lengths)"<<endl;
	cout<<endl;

	cout<<"  Partition"<<endl;
	cout<<endl;
	cout<<"     RayOutput/NumberOfSequences.txt"<<endl;
	cout<<"         Number of reads in each file"<<endl;
	cout<<"     RayOutput/SequencePartition.txt"<<endl;
	cout<<"     	Sequence partition"<<endl;
	cout<<endl;

	cout<<"  Ray software"<<endl;
	cout<<endl;
	cout<<"     RayOutput/RayVersion.txt"<<endl;
	cout<<"     	The version of Ray"<<endl;
	cout<<"     RayOutput/RayCommand.txt"<<endl;
	cout<<"     	The exact same command provided "<<endl;
	cout<<endl;

	cout<<"  AMOS"<<endl;
	cout<<endl;
	cout<<"     RayOutput/AMOS.afg"<<endl;
	cout<<"     	Assembly representation in AMOS format, required option: -amos"<<endl;
	cout<<endl;


	cout<<"  Communication"<<endl;
	cout<<endl;
	cout<<"     RayOutput/MessagePassingInterface.txt"<<endl;
	cout<<"	    	Number of messages sent"<<endl;
	cout<<"     RayOutput/NetworkTest.txt"<<endl;
	cout<<"	    	Latencies in microseconds"<<endl;
	cout<<"     RayOutput/Rank<rank>NetworkTestData.txt"<<endl;
	cout<<"	    	Network test raw data"<<endl;
	cout<<endl;

	cout<<"DOCUMENTATION"<<endl;
	cout<<endl;
	cout<<basicSpaces<<"- mpiexec -n 1 Ray -help|less (always up-to-date)"<<endl;
	cout<<basicSpaces<<"- This help page (always up-to-date)"<<endl;
	cout<<basicSpaces<<"- The directory Documentation/"<<endl;
 
	cout<<basicSpaces<<"- Manual (Portable Document Format): InstructionManual.tex (in Documentation)"<<endl;
	cout<<basicSpaces<<"- Mailing list archives: http://sourceforge.net/mailarchive/forum.php?forum_name=denovoassembler-users"<<endl;
	cout<<endl;
	cout<<"AUTHOR"<<endl;
	cout<<basicSpaces<<"Written by Sébastien Boisvert."<<endl;
	cout<<endl;
	cout<<"REPORTING BUGS"<<endl;
	cout<<basicSpaces<<"Report bugs to denovoassembler-users@lists.sourceforge.net"<<endl;
	cout<<basicSpaces<<"Home page: <http://denovoassembler.sourceforge.net/>"<<endl;
	cout<<endl;
	cout<<"COPYRIGHT"<<endl;
	cout<<basicSpaces<<"This program is free software: you can redistribute it and/or modify"<<endl;
	cout<<basicSpaces<<"it under the terms of the GNU General Public License as published by"<<endl;
	cout<<basicSpaces<<"the Free Software Foundation, version 3 of the License."<<endl;

	cout<<endl;
	cout<<basicSpaces<<"This program is distributed in the hope that it will be useful,"<<endl;
	cout<<basicSpaces<<"but WITHOUT ANY WARRANTY; without even the implied warranty of"<<endl;
	cout<<basicSpaces<<"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"<<endl;
	cout<<basicSpaces<<"GNU General Public License for more details."<<endl;

	cout<<endl;
	cout<<basicSpaces<<"You have received a copy of the GNU General Public License"<<endl;
	cout<<basicSpaces<<"along with this program (see LICENSE)."<<endl;

	cout<<endl;
	cout<<"Ray "<<RAY_VERSION<<endl;
}

/* get the prefix for mmap'ed files 
 * currently not used. */
string Parameters::getMemoryPrefix(){
	return m_memoryFilePrefix;
}

int Parameters::getReducerValue(){
	return m_reducerPeriod;
}

Rank Parameters::getRankFromGlobalId(ReadHandle a){
	uint64_t elementsPerRank=m_totalNumberOfSequences/m_size;
	Rank rank=a/elementsPerRank;

	if(rank >= m_size){
		rank=m_size-1;
	}

	#ifdef ASSERT

	assert(rank>=0);

	if(rank>=m_size){
		cout<<"GlobalIdentifier="<<a<<" Total="<<m_totalNumberOfSequences<<" Size="<<m_size<<" Rank="<<rank<<endl;
	}

	assert(rank<m_size);
	#endif

	return rank;
}

int Parameters::getIdFromGlobalId(ReadHandle a){
	int bin=getRankFromGlobalId(a);
	LargeCount x=m_totalNumberOfSequences/m_size;
	return a-bin*x;
}

int Parameters::getMaximumDistance(){
	return m_maximumDistance;
}

uint64_t Parameters::getGlobalIdFromRankAndLocalId(Rank rank,int id){
	uint64_t x=m_totalNumberOfSequences/m_size;
	return rank*x+id;
}

CoverageDepth Parameters::getMinimumCoverage(){
	return m_minimumCoverage;
}

void Parameters::setMinimumCoverage(CoverageDepth a){
	if(!m_providedMinimumCoverage)
		m_minimumCoverage=a;
}

Kmer Parameters::_complementVertex(Kmer*a){
	return a->complementVertex(m_wordSize,m_colorSpaceMode);
}

bool Parameters::hasPairedReads(){
	return m_numberOfLibraries!=0;
}

int Parameters::_vertexRank(Kmer*a){
	return a->vertexRank(m_size,m_wordSize,m_colorSpaceMode);
}

string Parameters::getScaffoldFile(){
	ostringstream a;
	a<<getPrefix()<<"Scaffolds.fasta";
	return a.str();
}

int Parameters::getColumns(){
	return 60;
}

int Parameters::getLargeContigThreshold(){
	return 500;
}

bool Parameters::showMemoryAllocations(){
	return m_showMemoryAllocations;
}

bool Parameters::writeKmers(){
	return m_writeKmers;
}

void Parameters::fileNameHook(string fileName){
	if(fileName.find(".csfasta")!=string::npos){
		if(!m_colorSpaceMode&&m_rank==MASTER_RANK){
			cout<<endl;
			cout<<"Enabling color-space mode"<<endl;
			cout<<"All reads should be in color space."<<endl;
		}
		m_colorSpaceMode=true;
	}
}

CoverageDepth Parameters::getMinimumCoverageToStore(){
	return 2;
}

int Parameters::getLibraryPeaks(int library){
	return m_libraryAverageLength[library].size();
}

bool Parameters::hasOption(string a){
	return m_options.count(a)>0;
}

bool Parameters::hasFile(const char*file){
	ifstream f(file);
	bool fileIsOk=f.good();
	f.close();
	return fileIsOk;
}

string Parameters::getCheckpointFile(const char*checkpointName){
	ostringstream a;
	a<<m_checkpointDirectory<<"/";
	a<<"Rank"<<getRank()<<".Checkpoint."<<checkpointName<<".ray";
	return a.str();
}

bool Parameters::hasCheckpoint(const char*checkpointName){
	//cout<<"hasCheckpoint? "<<checkpointName<<endl;

	if(!readCheckpoints())
		return false;

	return hasFile(getCheckpointFile(checkpointName).c_str());
}

bool Parameters::writeCheckpoints(){
	if(hasOption("-write-checkpoints"))
		return true;

	if(hasOption("-read-write-checkpoints"))
		return true;
	return false;
}

bool Parameters::readCheckpoints(){
	if(hasOption("-read-checkpoints"))
		return true;
	if(hasOption("-read-write-checkpoints"))
		return true;
	return false;
}

bool Parameters::showCommunicationEvents(){
	return m_showCommunicationEvents;
}

bool Parameters::showReadPlacement(){
	return m_showReadPlacement;
}

string Parameters::getConnectionType(){
	return m_connectionType;
}

int Parameters::getRoutingDegree(){
	return m_degree;
}

vector<string>*Parameters::getSearchDirectories(){
	return &m_searchDirectories;
}

string Parameters::getGenomeToTaxonFile(){
	return m_genomeToTaxonFile;
}

string Parameters::getTreeFile(){
	return m_treeFile;
}

string Parameters::getTaxonNameFile(){
	return m_taxonNameFile;
}

string Parameters::getSampleName(){
	string sample=getPrefix();

	return getBaseName(sample);
}

string Parameters::getBaseName(string directory){

	int last=directory.length()-1;
	while(last>0 && isDirectorySeparator(directory[last])){
		last--;
	}

	int first=last;
	while(first>0 && !isDirectorySeparator(directory[first])){
		first--;
	}

	if(isDirectorySeparator(directory[first])){
		first++;
	}

	int length=last-first+1;

	return directory.substr(first,length);
}

bool Parameters::isDirectorySeparator(char a){
	/* POSIX or Redmond separator */
	return a=='/' || a=='\\';
}

CoverageDepth Parameters::getMaximumSeedCoverage(){
	return m_maximumSeedCoverage;
}


uint64_t Parameters::getNumberOfBuckets(){

	uint64_t buckets=__DEFAULT_BUCKETS;

	if(hasConfigurationOption("-hash-table-buckets",1))
		return getConfigurationInteger("-hash-table-buckets",0);

	return buckets;
}

int Parameters::getNumberOfBucketsPerGroup(){

	int bucketsPerGroup=__DEFAULT_BUCKETS_PER_GROUP;

	if(hasConfigurationOption("-hash-table-buckets-per-group",1))
		return getConfigurationInteger("-hash-table-buckets-per-group",0);

	return bucketsPerGroup;
}

double Parameters::getLoadFactorThreshold(){

	double loadFactorThreshold=__DEFAULT_LOAD_FACTOR_THRESHOLD;

	if(hasConfigurationOption("-hash-table-load-factor-threshold",1))
		return getConfigurationDouble("-hash-table-load-factor-threshold",0);

	return loadFactorThreshold;
}

bool Parameters::hasConfigurationOption(const char*string,int count){
	for(int i=0;i<(int)m_commands.size();i++){
		if(strcmp(m_commands[i].c_str(),string)==0){
			int left=m_commands.size()-1-i;

			return left>=count;
		}
	}

	return false;
}

double Parameters::getConfigurationDouble(const char*string,int offset){

	#ifdef ASSERT
	assert(hasConfigurationOption(string,offset+1));
	#endif

	for(int i=0;i<(int)m_commands.size();i++){
		if(strcmp(m_commands[i].c_str(),string)==0){

			istringstream buffer(m_commands[i+1+offset]);

			double value=-1;
			buffer>>value;

			return value;
		}
	}

	return 0.0;

}

const char*Parameters::getConfigurationString(const char*string,int offset){

	#ifdef ASSERT
	assert(hasConfigurationOption(string,offset+1));
	#endif

	for(int i=0;i<(int)m_commands.size();i++){
		if(strcmp(m_commands[i].c_str(),string)==0){

			return m_commands[i+1+offset].c_str();
		}
	}

	return NULL;
}


uint64_t Parameters::getConfigurationInteger(const char*string,int offset){
	#ifdef ASSERT
	assert(hasConfigurationOption(string,offset+1));
	#endif

	for(int i=0;i<(int)m_commands.size();i++){
		if(strcmp(m_commands[i].c_str(),string)==0){

			istringstream buffer(m_commands[i+offset+1]);

			uint64_t value=-1;
			buffer>>value;

			return value;
		}
	}

	return 0;
}

void Parameters::__shuffleOperationCodes(){
	/* shuffle randomly arguments */

	vector<vector<string> > opCodes;
	int i=0;
	while(i<(int)m_commands.size()){
		int j=i;
		while(j+1<(int)m_commands.size() && m_commands[j+1][0]!='-'){
			j++;
		}
		vector<string> opCode;
		opCode.push_back(m_commands[i]);
		for(int k=i+1;k<=j;k++){
			opCode.push_back(m_commands[k]);
		}
		opCodes.push_back(opCode);
		i=j+1;
	}

	vector<int> indexes;
	getIndexes(opCodes.size(),&indexes);

	vector<string> newCommands;

	for(int i=0;i<(int)indexes.size();i++){
		for(int j=0;j<(int)opCodes[indexes[i]].size();j++){
			newCommands.push_back(opCodes[indexes[i]][j]);
		}
	}

	#ifdef ASSERT
	assert(newCommands.size()==m_commands.size());
	#endif

	m_commands=newCommands;
	newCommands.clear();
	opCodes.clear();

	if(getRank() == MASTER_RANK){
		cout<<"Rank 0: Shuffled opcodes"<<endl;
		for(int i=0;i<(int)m_commands.size();i++){
			cout<<" "<<m_commands[i]<<" \\"<<endl;
		}
		cout<<endl;
	}

}

bool Parameters::isValidInteger(const char*textMessage){
	int theLength=strlen(textMessage);

	if(!theLength)
		return false;

	int i=0;

	while(i<theLength){
		char symbol=textMessage[i++];
		if(!('0' <= symbol && symbol <= '9'))
			return false;
	}

	return true;
}
