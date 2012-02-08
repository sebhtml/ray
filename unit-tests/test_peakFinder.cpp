#include <plugin_Library/LibraryPeakFinder.h>
#include <unit-tests/unitTest.h>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

int main(int argc,char**argv){
	char*dataFile=argv[1];
	char*expectedFile=argv[2];

	vector<int> expectedAverages;
	vector<int> expectedDeviations;

	ifstream f1(expectedFile);
	int count;
	string buffer;
	f1>>buffer>>count;

	for(int i=0;i<count;i++){
		int average;
		int deviation;
		f1>>buffer>>buffer;
		f1>>buffer>>average;
		f1>>buffer>>deviation;

		expectedAverages.push_back(average);
		expectedDeviations.push_back(deviation);
	}
	f1.close();

	ifstream f(dataFile);
	vector<int> x;
	vector<int> y;
	while(!f.eof()){
		int insert=0;
		int frequency=0;
		f>>insert>>frequency;
		if(insert>0){
			x.push_back(insert);
			y.push_back(frequency);
		}
	}

	LibraryPeakFinder a;
	vector<int> averages;
	vector<int> deviations;
	a.findPeaks(&x,&y,&averages,&deviations);

	f.close();

	assertEquals(averages.size(),deviations.size());
	assertEquals(averages.size(),expectedAverages.size());

	for(int i=0;i<averages.size();i++){
		int expectedDown=expectedAverages[i]-expectedDeviations[i];
		int expectedUp=expectedAverages[i]+expectedDeviations[i];
		
		assertIsLower(expectedUp,averages[i]);

		assertIsGreater(expectedDown,averages[i]);
	}


	return 0;
}

