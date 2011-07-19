#include <heuristics/RayNovaEngine.h>
#include <map>
#include <vector>
#include <iostream>
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
int main(){
	RayNovaEngine m_novaEngine;
	ifstream f("nova.txt");
	while(!f.eof()){
		string buffer;
		f>>buffer;
		if(buffer=="RayNovaEngine"){
			cout<<endl;
			cout<<"Testing Entry with NovaEngine."<<endl;
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
					data[distance]=weight;
				}
				novaData.push_back(data);
			}
			m_novaEngine.choose(&novaData);
		}
	}
	f.close();
	return 0;
}
