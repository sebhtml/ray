/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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


#include<iostream>
#include<sstream>
#include<stdlib.h>
#include<fstream>
#include<set>
#include<string>
#include<Loader.h>
#include<common_functions.h>
#include<Read.h>
using namespace std;

int main(int argc,char**argv){
	if(argc!=5){
		cout<<"Usage:"<<endl;
		cout<<argv[0]<<" File.fasta fragmentSize coverage readSize"<<endl;
		cout<<" examples:"<<endl;
		return 0;
	}
	ran_seed();
	vector<Read*> sequences;
	string genomeFile=argv[1];
	int fragmentSize=atoi(argv[2]);
	int readSize=atoi(argv[4]);

	SequenceAllocator a;
	Allocator<Read> b;
	Loader l;
	l.load(genomeFile,&sequences,&a,&b);
	
	int coverage=atoi(argv[3]);

	ostringstream f1Name;
	f1Name<<fragmentSize<<"x"<<genomeFile;
	f1Name<<"_fragments_1.fasta";
	ofstream f1(f1Name.str().c_str());


	ostringstream f2Name;
	f2Name<<fragmentSize<<"x"<<genomeFile;
	f2Name<<"_fragments_2.fasta";
	ofstream f2(f2Name.str().c_str());
	int theReadNumber=0;
	for(int i=0;i<(int)sequences.size();i++){
		string sequence=sequences.at(i)->getSeq();
		int numberOfReads=sequence.length()*coverage/(2*readSize);
		for(int j=0;j<numberOfReads;j++){
			if(j%1000==0){
				cout<<j<<endl;
			}
			int start=ran_uniform()*(sequence.length()-fragmentSize+1);
			//int start=j;
			if(start<0)
				start=0;
			if(start>(int)sequence.length()-1){
				start=sequence.length()-1;
			}
			string fragment=sequence.substr(start,fragmentSize);


			if(rand()%2==0){
				fragment=reverseComplement(fragment);
			}
			int readNumber=theReadNumber;
			theReadNumber++;
			string read1=fragment.substr(0,readSize);
			string read2=reverseComplement(fragment).substr(0,readSize);
			f1<<">r_"<<start<<"_"<<readNumber<<"_1"<<endl<<read1<<endl;
			f2<<">r_"<<start<<"_"<<readNumber<<"_2"<<endl<<read2<<endl;
		}
	}
}

