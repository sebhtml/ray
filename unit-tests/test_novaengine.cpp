#include <plugin_SeedExtender/NovaEngine.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <unit-tests/unitTest.h>
#include <assert.h>
#include <fstream>
using namespace std;

/*
 * example

RayNovaEngine
Choices: 2
Choice: 1
 DataPoints: 18
 230 1 227 1 220 1 216 1 204 1 198 1 196 2 187 1 175 1 166 1 8 2 7 2 6 5 5 3 4 3 3 1 2 2 1 5
Choice: 2
 DataPoints: 3
 224 1 209 1 3 2

*/
int main(int argc,char**argv){
	NovaEngine m_novaEngine;
	set<int> invalid;
	char*fileName=argv[1];
	ifstream f(fileName);
	int passed=0;
	int failed=0;
	int test=0;
	while(!f.eof()){
		string buffer="";
		f>>buffer;
		if(buffer=="")
			break;
		if(buffer=="RayNovaEngine"){
			f>>buffer;
			int choices;
			f>>choices;
			vector<map<int,int> > novaData;
			for(int i=0;i<choices;i++){
				f>>buffer>>buffer>>buffer;
				int points;
				f>>points;
				map<int,int> data;
				for(int j=0;j<points;j++){
					int distance;
					int weight;
					f>>distance>>weight;
					data[distance]+=weight;
				}
				novaData.push_back(data);
			}
			int choice=m_novaEngine.choose(&novaData,&invalid,false);
			string expected;
			f>>expected>>expected;
			if(expected=="IMPOSSIBLE_CHOICE"){
				if(IMPOSSIBLE_CHOICE!=choice){
					failed++;
					m_novaEngine.choose(&novaData,&invalid,true);
				}else{
					passed++;
				}
				assertEquals(IMPOSSIBLE_CHOICE,choice);
			}else{
				int expectedChoice=atoi(expected.c_str())-1;
				if(expectedChoice!=choice){
					failed++;
					cout<<" test="<<test<<endl;
					m_novaEngine.choose(&novaData,&invalid,true);
				}else{
					passed++;
				}
				assertEquals(expectedChoice,choice);
			}
			test++;
		}
	}
	f.close();

	cout<<argv[0]<<" File: "<<fileName<<" Passed tests: "<<passed<<", Failed tests: "<<failed<<endl;

	return 0;
}
