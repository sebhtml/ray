#include <plugin_Library/LibraryPeakFinder.h>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

int main(int argc,char**argv){
	char*dataFile=argv[1];

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

	for(int i=0;i<averages.size();i++){
		cout<<"Peak "<<i<<" "<<averages[i]<<" "<<deviations[i]<<endl;
	}

	f.close();

	return 0;
}

