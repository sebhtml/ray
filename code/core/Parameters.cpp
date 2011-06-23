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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<core/common_functions.h>
#include<assert.h>
#include<math.h>
#include<core/Parameters.h>
#include<string>
#include<sstream>
#include<iostream>
#include<vector>
#include<cstdlib>
#include<fstream>
#include<structures/Read.h>
#include<assembler/Loader.h>
#include <memory/MyAllocator.h>
using namespace std;

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
	m_showMemoryUsage=false;
	m_showEndingContext=false;
	m_writeKmers=false;
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
	ifstream f(file);
	while(!f.eof()){
		string token="";
		f>>token;
		if(token!=""){
			m_commands.push_back(token);
		}
	}
	f.close();
}

void Parameters::loadCommandsFromArguments(int argc,char**argv){
	for(int i=0;i<argc;i++){
		m_commands.push_back(argv[i]);
	}
}

/* parse commands */
void Parameters::parseCommands(){
	m_initiated=true;
	set<string> commands;

	if(m_rank==MASTER_RANK){
		cout<<endl;
		cout<<"Ray command:"<<endl<<endl;

		for(int i=0;i<(int)m_commands.size();i++){
			if(i!=(int)m_commands.size()-1){
				cout<<" "<<m_commands[i]<<" \\"<<endl;
			}else{
				cout<<" "<<m_commands[i]<<endl;
			}
		}

		cout<<endl;
		cout<<endl;
		cout<<endl;
	}

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
	outputAmosCommands.insert("OutputAmosFile");
	outputAmosCommands.insert("-OutputAmosFile");
	outputAmosCommands.insert("--OutputAmosFile");

	set<string> showMalloc;
	showMalloc.insert("-show-memory-allocations");

	set<string> outputFileCommands;
	outputFileCommands.insert("-o");
	outputFileCommands.insert("OutputFile");
	outputFileCommands.insert("-OutputFile");
	outputFileCommands.insert("--OutputFile");
	
	set<string> memoryMappedFileCommands;
	memoryMappedFileCommands.insert("-MemoryPrefix");

	set<string> kmerSetting;
	kmerSetting.insert("-k");

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

	set<string> writeKmers;
	writeKmers.insert("-write-kmers");

	set<string> setMinimumCoverage;
	set<string> setPeakCoverage;
	set<string> setRepeatCoverage;
	setMinimumCoverage.insert("-minimumCoverage");
	setPeakCoverage.insert("-peakCoverage");
	setRepeatCoverage.insert("-repeatCoverage");

	vector<set<string> > toAdd;
	toAdd.push_back(setRepeatCoverage);
	toAdd.push_back(setPeakCoverage);
	toAdd.push_back(setMinimumCoverage);
	toAdd.push_back(singleReadsCommands);
	toAdd.push_back(pairedReadsCommands);
	toAdd.push_back(outputAmosCommands);
	toAdd.push_back(outputFileCommands);
	toAdd.push_back(kmerSetting);
	toAdd.push_back(interleavedCommands);
	toAdd.push_back(reduceMemoryUsage);
	toAdd.push_back(memoryMappedFileCommands);
	toAdd.push_back(showMemory);	
	toAdd.push_back(debugBubbles);
	toAdd.push_back(debugSeeds);
	toAdd.push_back(runProfiler);
	toAdd.push_back(showContext);
	toAdd.push_back(showMalloc);
	toAdd.push_back(writeKmers);
	toAdd.push_back(colorSpaceMode);

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
				if(commands.count(cmd)==0){
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
		}else if(pairedReadsCommands.count(token)>0){
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
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

			if(items==4){
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
			}else if(items==2){// automatic detection.
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

	if(m_rank==MASTER_RANK){
		ostringstream commandFile;
		commandFile<<getPrefix()<<".RayCommand.txt";
		ofstream f(commandFile.str().c_str());
		f<<"mpirun -np "<<getSize()<<" \\"<<endl;
		for(int i=0;i<(int)m_commands.size();i++){
			if(i!=(int)m_commands.size()-1){
				f<<" "<<m_commands[i]<<" \\"<<endl;
			}else{
				f<<" "<<m_commands[i]<<endl;
			}
		}
		f.close();

		cout<<endl;
		cout<<"k-mer length: "<<m_wordSize<<endl;
		
		if(m_reducerIsActivated){
			cout<<"Memory Consumption Reducer is enabled, threshold="<<m_reducerPeriod<<endl;
		}
		cout<<endl;
		cout<<"Output files will be prefixed with "<<getPrefix()<<endl;
		cout<<endl;

		ostringstream rayRuntime;
		rayRuntime<<getPrefix()<<".RayVersion.txt";
		ofstream f2(rayRuntime.str().c_str());
		f2<<"Ray version: "<<RAY_VERSION<<endl;
		f2.close();
	}

	int maximumNumberOfFiles=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint32_t);
	
	assert((int)m_singleEndReadsFile.size()<=maximumNumberOfFiles);

	uint64_t result=1;
	for(int p=0;p<m_wordSize;p++){
		result*=4;
	}
}

void Parameters::constructor(int argc,char**argv,int rank){
	m_maximumDistance=0;
	m_totalNumberOfSequences=0;

	m_rank=rank;
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

string Parameters::getDirectory(){
	return m_directory;
}

string Parameters::getOutputFile(){
	return getPrefix()+".Contigs.fasta";
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

int Parameters::getLibraryAverageLength(int i){
	return m_libraryAverageLength[i];
}

int Parameters::getLibraryStandardDeviation(int i){
	return m_libraryDeviation[i];
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
	return m_prefix;
}

string Parameters::getCoverageDistributionFile(){
	return getPrefix()+".CoverageDistribution.txt";
}

string Parameters::getAmosFile(){
	return getPrefix()+".AMOS.afg";
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
	s<<"."<<"Library"<<library<<".txt";
	return s.str();
}

#define WRITE_LIBRARY_OBSERVATIONS

void Parameters::computeAverageDistances(){
	cout<<endl;
	for(map<int,map<int,int> >::iterator i=m_observedDistances.begin();
		i!=m_observedDistances.end();i++){
		uint64_t sum=0;
		int library=i->first;
		int n=0;
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
			sum+=d*count;
			n+=count;
		}
		#ifdef WRITE_LIBRARY_OBSERVATIONS
		f.close();
		#endif
		int average;
		int standardDeviation;
		if(n>0){
			average=sum/n;
			sum=0;
			for(map<int,int>::iterator j=m_observedDistances[library].begin();
				j!=m_observedDistances[library].end();j++){
				int d=j->first;
				int count=j->second;
				int diff=d-average;
				sum+=diff*diff*count;
			}
			sum/=n;
			standardDeviation=(int)sqrt((double)1.0*sum);
		}else{
			average=0;
			standardDeviation=0;
		}

		addLibraryData(library,average,standardDeviation);

	}	
	cout<<endl;
	cout<<endl;

	ostringstream fileName;
	fileName<<getPrefix();
	fileName<<".LibraryStatistics.txt";
	ofstream f2(fileName.str().c_str());

	uint64_t totalSequences=0;
	for(int i=0;i<(int)m_singleEndReadsFile.size();i++){
		#ifdef ASSERT
		assert(i<(int)m_singleEndReadsFile.size());
		#endif
		f2<<" File: "<<m_singleEndReadsFile[i]<<endl;
		f2<<"  NumberOfSequences: "<<m_numberOfSequencesInFile[i]<<endl;
		totalSequences+=m_numberOfSequencesInFile[i];
	}
	f2<<endl<<"Total: "<<totalSequences<<endl;
	f2<<endl;

	f2<<"NumberOfPairedLibraries: "<<m_numberOfLibraries<<endl;
	f2<<endl;
	for(int i=0;i<(int)m_numberOfLibraries;i++){
		int library=i;
		string type="Manual";
		if(m_automaticLibraries.count(library)>0){
			type="Automatic";
		}
		int average=getLibraryAverageLength(library);
		int standardDeviation=getLibraryStandardDeviation(library);
		cout<<"Library # "<<library<<" ("<<type<<") -> average length: "<<average<<" and standard deviation: "<<standardDeviation<<endl;
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
		f2<<" AverageOuterDistance: "<<average<<endl;
		f2<<" StandardDeviation: "<<standardDeviation<<endl;
		if(standardDeviation*2>average){
			f2<<" DetectionFailure: Yes"<<endl;
		}
		f2<<endl;
	}
	f2.close();
}

void Parameters::addLibraryData(int library,int average,int deviation){
	m_libraryAverageLength[library]=average;
	m_libraryDeviation[library]=deviation;
	
	int distance=average+deviation;
	if(distance>m_maximumDistance){
		m_maximumDistance=distance;
	}
}

void Parameters::setNumberOfSequences(uint64_t n){
	if((int)m_numberOfSequencesInFile.size()<getNumberOfFiles()){
		m_totalNumberOfSequences+=n;
	}

	m_numberOfSequencesInFile.push_back(n);
}

int Parameters::getNumberOfLibraries(){
	return m_numberOfLibraries;
}

uint64_t Parameters::getNumberOfSequences(int file){
	#ifdef ASSERT
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
	string outputForMessages=getPrefix()+".ReceivedMessages.txt";
	return outputForMessages;
}

void Parameters::printFinalMessage(){
	cout<<"Rank "<<MASTER_RANK<<" wrote library statistics"<<endl;
}

int Parameters::getMaximumAllowedCoverage(){
	COVERAGE_TYPE a=0;
	a--;
	return a;
}

void Parameters::setPeakCoverage(int a){
	if(!m_providedPeakCoverage)
		m_peakCoverage=a;
}

void Parameters::setRepeatCoverage(int a){
	if(!m_providedRepeatCoverage)
		m_repeatCoverage=a;
}

int Parameters::getPeakCoverage(){
	return m_peakCoverage;
}

int Parameters::getRepeatCoverage(){
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
	cout<<basicSpaces<<"mpirun -np NUMBER_OF_RANKS Ray -p l1_1.fastq l1_2.fastq -p l2_1.fastq l2_2.fastq"<<endl;
	cout<<endl;
	cout<<"DESCRIPTION:"<<endl;
	showOption("-k kmerLength","Selects the length of k-mers. The default value is 21. ");
	showOptionDescription("It must be odd because reverse-complement vertices are stored together.");
	showOptionDescription("The maximum length is defined at compilation by MAXKMERLENGTH");
	showOptionDescription("Larger k-mers utilise more memory.");
	cout<<endl;

	showOption("-p leftSequenceFile rightSequenceFile [averageOuterDistance standardDeviation]","Provides two files containing paired-end reads.");
	showOptionDescription("averageOuterDistance and standardDeviation are automatically computed if not provided.");
	cout<<endl;
	showOption("-i interleavedSequenceFile [averageOuterDistance standardDeviation]","Provides one file containing interleaved paired-end reads.");

	showOptionDescription("averageOuterDistance and standardDeviation are automatically computed if not provided.");
	cout<<endl;

	showOption("-s sequenceFile","Provides a file containing single-end reads.");
	cout<<endl;
	showOption("-o outputPrefix","Specifies the prefix for outputted files.");
	cout<<endl;
	showOption("-amos","Writes the AMOS file called PREFIX.AMOS.afg");
	showOptionDescription("An AMOS file contains read positions on contigs.");
	showOptionDescription("Can be opened with software with graphical user interface.");
	cout<<endl;
	showOption("-write-kmers","Writes k-mer graph to PREFIX.kmers.txt");
	showOptionDescription("The resulting file is not utilised by Ray.");
	showOptionDescription("The resulting file is very large.");
	cout<<endl;
	showOption("-run-profiler","Runs the profiler as the code runs. Needs a real-time POSIX system.");
	showOptionDescription("Needs HAVE_CLOCK_GETTIME=y at compilation");
	showOptionDescription("Running the profiler increases running times.");
	cout<<endl;
	showOption("-color-space","Runs in color-space");
	showOptionDescription("Needs csfasta files. Activated automatically if csfasta files are provided.");
	cout<<endl;
	showOption("-minimumCoverage minimumCoverage","Sets manually the minimum coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
	showOption("-peakCoverage peakCoverage","Sets manually the peak coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
	showOption("-repeatCoverage repeatCoverage","Sets manually the repeat coverage.");
	showOptionDescription("If not provided, it is computed by Ray automatically.");
	cout<<endl;
	showOption("-debug-bubbles","Debugs bubble code.");
	showOptionDescription("Bubbles can be due to heterozygous sites or sequencing errors or other (unknown) events");
	cout<<endl;
	showOption("-debug-seeds","Debugs seed code.");
	showOptionDescription("Seeds are paths in the graph that are likely unique.");
	cout<<endl;
	showOption("-show-memory-usage","Shows memory usage. Data is fetched from /proc on GNU/Linux");
	showOptionDescription("Needs __linux__; Lines with string 'VmData' are outputted");
	cout<<endl;
	showOption("-show-ending-context","Shows the ending context of each extension.");
	showOptionDescription("Shows the children of the vertex where extension was too difficult.");
	cout<<endl;
	showOption("-show-memory-allocations","Shows memory allocation events");
	cout<<endl;
	showOption("-help","Displays this help page.");
	cout<<endl;

	cout<<"FILES"<<endl;

	cout<<"     .fasta"<<endl;
	cout<<"     .fasta.gz (needs HAVE_LIBZ=y at compilation)"<<endl;
	cout<<"     .fasta.bz2 (needs HAVE_LIBBZ2=y at compilation)"<<endl;
	cout<<"     .fastq"<<endl;
	cout<<"     .fastq.gz (needs HAVE_LIBZ=y at compilation)"<<endl;
	cout<<"     .fastq.bz2 (needs HAVE_LIBBZ2=y at compilation)"<<endl;
	cout<<"     .sff (paired reads must be extracted manually)"<<endl;
	cout<<"     .csfasta (color-space reads)"<<endl;

	cout<<endl;
	cout<<"     Note: file format is determined with file extension."<<endl;
	cout<<endl;
	cout<<"DOCUMENTATION"<<endl;
	cout<<basicSpaces<<"Manual (Portable Document Format): InstructionManual.pdf"<<endl;
	cout<<basicSpaces<<"Manual (online): <http://denovoassembler.sf.net/manual.html>"<<endl;
	cout<<basicSpaces<<"github README.md: <http://github.com/sebhtml/ray>"<<endl;
	cout<<basicSpaces<<"This help page"<<endl;
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

int Parameters::getRankFromGlobalId(uint64_t a){
	uint64_t elementsPerRank=m_totalNumberOfSequences/m_size;
	int rank=a/elementsPerRank;
	if(rank==m_size){
		rank--;
	}
	#ifdef ASSERT
	if(rank>=m_size){
		cout<<"GlobalIdentifier="<<a<<" Total="<<m_totalNumberOfSequences<<" Size="<<m_size<<" Rank="<<rank<<endl;
	}
	assert(rank<m_size);
	#endif
	return rank;
}

int Parameters::getIdFromGlobalId(uint64_t a){
	int bin=getRankFromGlobalId(a);
	uint64_t x=m_totalNumberOfSequences/m_size;
	return a-bin*x;
}

int Parameters::getMaximumDistance(){
	return m_maximumDistance;
}

uint64_t Parameters::getGlobalIdFromRankAndLocalId(int rank,int id){
	uint64_t x=m_totalNumberOfSequences/m_size;
	return rank*x+id;
}

int Parameters::getMinimumCoverage(){
	return m_minimumCoverage;
}

void Parameters::setMinimumCoverage(int a){
	if(!m_providedMinimumCoverage)
		m_minimumCoverage=a;
}

Kmer Parameters::_complementVertex(Kmer*a){
	return complementVertex(a,m_wordSize,m_colorSpaceMode);
}

bool Parameters::hasPairedReads(){
	return m_numberOfLibraries!=0;
}

int Parameters::_vertexRank(Kmer*a){
	return vertexRank(a,m_size,m_wordSize,m_colorSpaceMode);
}
int Parameters::getSlaveMode(){
	return *m_slaveMode;
}

void Parameters::setSlaveMode(int a){
	*m_slaveMode=a;
}

void Parameters::setSlaveModePointer(int*a){
	m_slaveMode=a;
}

int Parameters::getMasterMode(){
	return *m_masterMode;
}

void Parameters::setMasterMode(int a){
	*m_masterMode=a;
}

void Parameters::setMasterModePointer(int*a){
	m_masterMode=a;
}

string Parameters::getScaffoldFile(){
	ostringstream a;
	a<<getPrefix()<<".Scaffolds.fasta";
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
