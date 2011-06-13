#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <ctime>

using namespace std;

char getRandomNucleotide(){
	int a=rand()%4;
	switch (a){
		case 0:
			return 'A';
		case 1:
			return 'T';
		case 2:
			return 'C';
		case 3:
			return 'G';
	}
	return 'N';
}

string addSubstitutionErrors(string*a,double rate){
	string b=*a;
	int resolution=100000000;
	int length=b.length();
	int threshold=rate*resolution;
	for(int i=0;i<length;i++){
		int val=rand()%resolution;
		if(val<threshold){
			char oldNucleotide=b[i];
			char newNucleotide=getRandomNucleotide();
			while(newNucleotide==oldNucleotide){
				newNucleotide=getRandomNucleotide();
			}
			b[i]=newNucleotide;
		}
	}
	return b;
}

string reverseComplement(string*a){
	ostringstream b;
	int n=a->length()-1;
	while(n>=0){
		char symbol=a->at(n--);

		switch (symbol){
			case 'A':
				symbol='T';
				break;
			case 'T':
				symbol='A';
				break;
			case 'C':
				symbol='G';
				break;
			case 'G':
				symbol='C';
				break;
		}
		b<<symbol;
	}
	return b.str();
}

int main(int argc,char**argv){
	cout<<argv[0]<<" GENOME.FASTA SUBSTITUTION_RATE AVERAGE_OUTER_DISTANCE STANDARD_DEVIATION PAIRS READ_LENGTH OUT1.fasta OUT2.fasta"<<endl;
	if(argc!=9){
		cout<<argc<<" but needs 9"<<endl;
		return 0;
	}
	string file=argv[1];
	double substitutionRate=atof(argv[2]);
	int average=atoi(argv[3]);
	int dev=atoi(argv[4]);
	int pairs=atoi(argv[5]);
	int length=atoi(argv[6]);
	string left=argv[7];
	string right=argv[8];

	boost::mt19937 generator(time(NULL)+getpid());
	boost::normal_distribution<> distribution(average,dev);
	boost::variate_generator<boost::mt19937&,boost::normal_distribution<> > fragmentLengthSampler(generator,distribution);


	ostringstream genome;
	string header;
	ifstream f(file.c_str());
	while(!f.eof()){
		char line[1000000];
		int maxSize=1000000;
		f.getline(line,maxSize);
		if(line[0]!='>'){
			genome<<line;
		}else{
			header=line;
		}
	}
	f.close();
	cout<<header<<endl;
	string sequence=genome.str();
	int sequenceLength=sequence.length();

	boost::mt19937 generator2(time(NULL)+getpid()*3);
	boost::uniform_int<> uniformDistribution(0,sequenceLength-1);
	boost::variate_generator<boost::mt19937&,boost::uniform_int<> > fragmentPositionSampler(generator2,uniformDistribution);


	cout<<sequenceLength<<" nucleotides"<<endl;
	cout<<"AverageLength="<<average<<" StandardDeviation="<<dev<<endl;
	cout<<"Pairs="<<pairs<<" ReadLength="<<length<<endl;
	cout<<"SubstitutionRate="<<substitutionRate<<endl;

	int i=0;

	srand(time(NULL)+getpid()+4);
	ofstream leftOut(left.c_str());
	ofstream rightOut(right.c_str());
	cout<<"OUTPUTS: "<<left<<" "<<right<<endl;
	while(i<pairs){
		if(i%100000==0){
			cout<<i+1<<"/"<<pairs<<endl;
		}
		int start=fragmentPositionSampler();
		int observedDistance=fragmentLengthSampler();

		if(start+observedDistance>sequenceLength){
			continue;
		}
		int reverse=rand()%2;
		string leftSequence=sequence.substr(start,length);
		string rightSequence=sequence.substr(start+observedDistance-length,length);
		if(reverse==1){
			rightSequence=reverseComplement(&rightSequence);
		}else{
			leftSequence=reverseComplement(&leftSequence);
		}
		leftSequence=addSubstitutionErrors(&leftSequence,substitutionRate);
		rightSequence=addSubstitutionErrors(&rightSequence,substitutionRate);

		leftOut<<">"<<i<<"/1"<<endl<<leftSequence<<endl;
		rightOut<<">"<<i<<"/2"<<endl<<rightSequence<<endl;
		i++;
	}
	cout<<i<<"/"<<pairs<<endl;
	cout<<"Done."<<endl;
	leftOut.close();
	rightOut.close();
	
	return EXIT_SUCCESS;
}
