
#include <plugin_Searcher/QualityCaller.h>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
using namespace std;

void load(map<int,uint64_t>*distro,const char*file){
	char line[4000];
	ifstream f(file);
	f.getline(line,4000);

	while(!f.eof()){
		int x=-1;
		uint64_t y=0;
		f>>x>>y;

		if(x!=-1)
			(*distro)[x]=y;
	}

	f.close();

	cout<<"Loaded "<<distro->size()<<endl;
}

int main(int argc,char**argv){

	string key=argv[1];

	map<int,uint64_t> d1;
	ostringstream n1;
	n1<<key<<".RawDistribution.tsv";
	load(&d1,n1.str().c_str());

	map<int,uint64_t> d2;
	ostringstream n2;
	n2<<key<<".UniquelyColoredDistribution.tsv";
	load(&d2,n2.str().c_str());

	map<int,uint64_t> d3;
	ostringstream n3;
	n3<<key<<".UniquelyColoredAssembledDistribution.tsv";
	load(&d3,n3.str().c_str());

	QualityCaller c;

	double q1=c.computeQuality(&d2,&d1);
	cout<<"UniquelyColoredDistribution and RawDistribution -> "<<q1<<endl;

	double q2=c.computeQuality(&d3,&d1);
	cout<<"UniquelyColoredAssembledDistribution and UniquelyColoredDistribution -> "<<q2<<endl;

	double q3=c.computeQuality(&d3,&d2);
	cout<<"UniquelyColoredAssembledDistribution and RawDistribution -> "<<q3<<endl;

	return 0;
}
