#include<string>
#include<string.h>
#include<fstream>
#include<sstream>
#include<iostream>
using namespace std;

void map(const string*ref,const char*read,const char*name,int ii){
	int bestScore=-1;
	int bestPosition=-1;
	int reflen=ref->length();
	int readlen=strlen(read);

	for(int i=0;i<reflen;i++){
		int score=0;
		for(int j=0;j<readlen;j++){
			int positionInRef=i+j;
			int positionInRead=j;
			if(positionInRef==reflen){
				break;
			}
			if(read[positionInRead]==ref->at(positionInRef)){
				score++;
			}
		}
		if(score>bestScore){
			bestScore=score;
			bestPosition=i;
		}
	}

	if(bestScore<25)
		return;

	cout<<endl;
	cout<<ii<<" "<<name<<" BestScore="<<bestScore<<" Position="<<bestPosition<<endl;
	cout<<ref->substr(bestPosition,readlen)<<endl;
	for(int j=0;j<readlen;j++){
		if(read[j]==ref->at(j+bestPosition)){
			cout<<"|";
		}else{
			cout<<" ";
		}
	}
	cout<<endl;
	cout<<read<<endl;

}

int main(){
	string reference="DH10B.csfasta";
	string readF="test.csfasta";
	ifstream f(reference.c_str());
	ostringstream s;
	while(!f.eof()){
		char line[4096];
		f.getline(line,4096);
		s<<line;
	}
	f.close();
	char read[1000];
	char reverse[1000];
	ifstream f2(readF.c_str());
	string ref=s.str();
	int i=1;
	while(!f2.eof()){
		f2.getline(read,1000);
		string name=read;
		f2.getline(read,1000);
		char*read2=read+2;
		for(int j=0;j<strlen(read2);j++){
			reverse[strlen(read2)-j-1]=read2[j];
		}
		reverse[strlen(read2)]='\0';
		map(&ref,read2,name.c_str(),i);
		map(&ref,reverse,name.c_str(),i);
		i++;
	}
	f2.close();

	return 0;
}
