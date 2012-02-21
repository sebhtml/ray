#include <iostream>
#include <plugin_Searcher/ColoredPeakFinder.h>
#include <fstream>
#include <vector>
#include <assert.h>
using namespace std;
#include <unit-tests/unitTest.h>

void run_test(string file,string result){

	cout<<"**************************************************"<<endl;

	cout<<"Test "<<file<<" expected "<<result<<endl;

	vector<int> x;
	vector<int> y;
	vector<int> peaks;
	vector<int> deviations;

	ifstream f(file.c_str());

	// skip the header
	char buffer4[1000];
	f.getline(buffer4,1000);

	while(!f.eof()){
		int x1=-1;
		int y1=-1;
		f>>x1>>y1;
		
		if(x1!=-1){
			x.push_back(x1);
			y.push_back(y1);
		}
	}

	f.close();

	cout<<x.size()<<" points"<<endl;

	ColoredPeakFinder object;

	object.findPeaks(&x,&y,&peaks,&deviations);

	if(result=="yes"){
		assertEquals(1,peaks.size());
	}else if(result=="no"){
		assertEquals(0,peaks.size());
	}else{
		assert(false);
	}

}

int main(int argc,char**argv){
	char*file=argv[1];

	int tests=0;

	ifstream f(file);

	while(!f.eof()){
		string buffer;
		f>>buffer;
		if(buffer=="File:")
			tests++;
	}

	f.close();

	f.open(file);

	cout<<"Tests: "<<tests<<endl;

	for(int i=0;i<tests;i++){
		string file;
		string result;
		f>>file>>file;
		f>>result>>result;

		run_test(file,result);
	}

	return 0;
}
