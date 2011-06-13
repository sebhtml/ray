#include <graph/CoverageDistribution.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
using namespace std;

int main(int argc,char**argv){
	string file=argv[1];
	map<int,uint64_t> data;
	ifstream f(file.c_str());
	while(!f.eof()){
		int a=-1;
		uint64_t b=0;
		f>>a>>b;
		if(a==-1){
			continue;
		}else if(b==0){
			continue;
		}else{
			data[a]=b;
		}
	}
	f.close();

	CoverageDistribution analysis(&data,NULL);
	cout<<"File= "<<file<<endl;
	cout<<"MinCoverage= "<<analysis.getMinimumCoverage()<<endl;
	cout<<"PeakCoverage= "<<analysis.getPeakCoverage()<<endl;
	cout<<"RepeatCoverage= "<<analysis.getRepeatCoverage()<<endl;

	return 0;
}
