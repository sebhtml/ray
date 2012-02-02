#include <plugin_CoverageGatherer/CoverageDistribution.h>
#include <string>
#include <map>
#include <unit-tests/unitTest.h>
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

	ifstream f2(argv[2]);
	int expectedPeak=-1;
	int expectedMin=-1;
	f2>>expectedMin;
	f2>>expectedPeak;
	CoverageDistribution analysis(&data,NULL);
	int actualMin=analysis.getMinimumCoverage();
	int actualPeak=analysis.getPeakCoverage();

	cout<<endl;
	cout<<"Test "<<file<<endl;
	cout<<"Expected minimum: "<<expectedMin<<" actual "<<actualMin<<endl;
	cout<<"Expected peak: "<<expectedPeak<<" actual "<<actualPeak<<endl;

	if(expectedPeak!=actualPeak || expectedMin!=actualMin)
		cout<<"Test "<<file<<" Failed"<<endl;
	else
		cout<<"Test "<<file<<" Passed"<<endl;

	assertEquals(expectedPeak,actualPeak);
	assertEquals(expectedMin,actualMin);

	return 0;
}
