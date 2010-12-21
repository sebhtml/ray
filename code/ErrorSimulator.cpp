/*
 	OpenAssembler -- a de Bruijn DNA assembler for mixed high-throughput technologies
    Copyright (C) 2009  Sébastien Boisvert

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


#include<ErrorSimulator.h>
#include<common_functions.h>
#include<MyAllocator.h>
#include<Read.h>
#include<stdlib.h>
#include<Loader.h>
#include<fstream>
using namespace std;

void ErrorSimulator::simulateErrors(string inputFile,string outputFile){
	Loader loader;
	MyAllocator a;
	a.constructor(4096);
	loader.load(inputFile,false);
	srand(time(NULL));

	double insertionProbability=0.005;
	double deletionProbability=0.005;
	double mismatchProbability=0.01;
	double unknownBaseProbability=0.005;

	cout<<"Insertion probability: "<<insertionProbability<<endl;
	cout<<"Deletion probability: "<<deletionProbability<<endl;
	cout<<"Mismatch base probability: "<<mismatchProbability<<endl;
	cout<<"Unknown base probability: "<<unknownBaseProbability<<endl;

	int probabilityPrecision=100000;
	ofstream f(outputFile.c_str());
	for(int i=0;i<loader.size();i++){
		string sequence=loader.at(i)->getSeq();
		if(i%10000==0)
			cout<<".";
		for(int position=0;position<(int)sequence.length();position++){
	
			if(rand()%probabilityPrecision<probabilityPrecision*mismatchProbability){
				simulateMismatch(&sequence,position);
			}

			continue;
			if(rand()%probabilityPrecision<probabilityPrecision*insertionProbability){
				simulateInsertion(&sequence,position);
			}


			if(rand()%probabilityPrecision<probabilityPrecision*unknownBaseProbability){
				simulateUnknownBase(&sequence,position);
			}
			if(rand()%probabilityPrecision<probabilityPrecision*deletionProbability){
				simulateDeletion(&sequence,position);
			}
		}
		int id=i;
		f<<">"<<id<<endl<<addLineBreaks(sequence);
	}
	cout<<endl;
	f.close();
	cout<<"Wrote "<<outputFile<<endl;
}

void ErrorSimulator::simulateDeletion(string*a,int i){
	(*a)=a->substr(0,i-1)+a->substr(i);
}

void ErrorSimulator::simulateInsertion(string*a,int i){
	(*a)=a->substr(0,i)+a->at(i)+a->substr(i);
}

void ErrorSimulator::simulateMismatch(string *a,int i){
	char oldChar=(*a)[i];
	int theCase=rand()%4;
	char newChar=oldChar;
	if(theCase==0){
		newChar='A';
	}else if(theCase==1){
		newChar='T';
	}else if(theCase==2){
		newChar='C';
	}else if(theCase==3){
		newChar='G';
	}
	if(newChar==oldChar){
		simulateMismatch(a,i);
	}else{
		(*a)[i]=newChar;
	}
}

void ErrorSimulator::simulateUnknownBase(string*a,int i){
	(*a)[i]='N';
}

