/*
 	Ray
    Copyright (C) 2009, 2010  Sébastien Boisvert

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

#include<vector>
#include<fstream>
#include<common_functions.h>
#include<stdlib.h>
#include<iostream>
#include<string>
#include<cstring>
#include<sstream>
using namespace std;

string reverseComplement(string a){
	char*rev=new char[a.length()+1];
	int i=0;
	for(int p=a.length()-1;p>=0;p--){
		char c=a[p];
		switch(c){
			case 'A':
				rev[i]='T';
				break;
			case 'T':
				rev[i]='A';
				break;
			case 'G':
				rev[i]='C';	
				break;
			case 'C':
				rev[i]='G';
				break;
			default:
				rev[i]=c;
				break;
		}
		i++;
	}
	rev[a.length()]='\0';
	string j(rev);
	delete[]rev;
	return j;
}

// convert k-mer to uint64_t
uint64_t wordId(const char*a){
	uint64_t i=0;
	for(int j=0;j<(int)strlen(a);j++){
		uint64_t k=0; // default is A
		char h=a[j];
		switch(h){
			case 'T':
				k=1;
				break;
			case 'C':
				k=2;
				break;
			case 'G':
				k=3;
				break;
		}
		i=(i|(k<<(j<<1)));
	}
	return i;
}

string idToWord(uint64_t i,int wordSize){
	char*a=new char[wordSize+1];
	for(int p=0;p<wordSize;p++){
		uint64_t j=(i<<(62-2*p))>>62;
		switch(j){
			case 0:
				a[p]='A';
				break;
			case 1:
				a[p]='T';
				break;
			case 2:
				a[p]='C';
				break;
			case 3:
				a[p]='G';
				break;
			default:
				break;
		}
	}
	a[wordSize]='\0';
	string b(a);
	delete[]a;
	return b;
}

//  63 62 ... 1 0
//              
char getFirstSymbol(uint64_t i,int k){
	i=(i<<(62))>>62;
        if((int)i==0)
                return 'A';
        if((int)i==1)
                return 'T';
        if((int)i==2)
                return 'C';
        if((int)i==3)
                return 'G';
	return '0';
}

char getLastSymbol(uint64_t i,int m_wordSize){
	i=(i<<(64-2*m_wordSize))>>62;
        if((int)i==0)
                return 'A';
        if((int)i==1)
                return 'T';
        if((int)i==2)
                return 'C';
        if((int)i==3)
                return 'G';
	cout<<"Fatal exception, getLastSymbol."<<endl;
	exit(0);
}

bool isValidDNA(const char*x){
	int len=strlen(x);
	for(int i=0;i<len;i++){
		char a=x[i];
		if(!(a=='A'||a=='T'||a=='C'||a=='G'))
			return false;
	}
	return true;
}


/*
 * 
 *   63 62 ... 1 0
 *
 */

void coutBIN(uint64_t a){
	for(int i=63;i>=0;i--){
		cout<<(int)((a<<(63-i))>>63);
	}
	cout<<endl;
}



uint64_t getKPrefix(uint64_t a,int k){
	return (a<<(64-2*(k+1)+2))>>(64-2*(k+1)+2);
}

uint64_t getKSuffix(uint64_t a,int k){
	return a>>2;
}



uint64_t complementVertex(uint64_t a,int m_wordSize){
	//  A:0, T:1, C:2, G:3
	uint64_t output=0;
	int position2=0;
	for(int positionInMer=m_wordSize-1;positionInMer>=0;positionInMer--){
		uint64_t j=(a<<(62-2*positionInMer))>>62;
		uint64_t complementVertex=0;
		switch(j){
			case 0:
				complementVertex=1;
				break;
			case 1:
				complementVertex=0;
				break;
			case 2:
				complementVertex=3;
				break;
			case 3:
				complementVertex=2;
				break;
			default:
				break;
		}
		output=(output|(complementVertex<<(position2<<1)));
		position2++;
	}
	return output;
}







string addLineBreaks(string dna){
	ostringstream output;
	int j=0;
	int columns=60;
	while(j<(int)dna.length()){
		output<<dna.substr(j,columns)<<endl;
		j+=columns;
	}
	return output.str();
}



// return the length of the longuest homopolymer in sequence as
int homopolymerLength(string*as){
	int max=1;
	int count=1;
	for(int i=0;i<(int)as->length()-1;i++){// avoid the last nucleotide
		if(i!=0){
			if(as->at(i-1)==as->at(i)){
				count++;
			}else{
				if(count>max){
					max=count;
				}	
				count=1;
			}
		}
	}
	if(count>max){
		max=count;
	}	

	return max;
}








bool fileExists(string a){
	#ifdef DEBUG_ASSEMBLER
cout<<"testing file: "<<a<<endl;
	#endif
	ifstream b(a.c_str());
	bool c=false;
	if(b)
		c=true;
	b.close();
	#ifdef DEBUG_ASSEMBLER
	cout<<c<<endl;
	#endif
	return c;
}




// http://www.concentric.net/~Ttwang/tech/inthash.htm 64 bit Mix Functions
uint64_t hash_uint64_t(uint64_t key){
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}
