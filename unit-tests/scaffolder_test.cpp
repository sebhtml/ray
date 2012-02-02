#include <plugin_Scaffolder/ScaffoldingAlgorithm.h>
#include <plugin_Scaffolder/ScaffoldingVertex.h>
#include <plugin_Scaffolder/ScaffoldingEdge.h>
#include <string>
#include <iostream>
using namespace std;

int main(int argc,char**argv){

	if(argc==1)
		return 0;

	string vertexFile=argv[1];
	string edgeFile=argv[2];

	vector<ScaffoldingVertex> vertices;
	vector<ScaffoldingEdge> edges;

	{
	ifstream f(vertexFile.c_str());

	if(!f)
		return 0;

	int n=0;
	while(!f.eof()){
		string token="";
		f>>token;
		if(token!="")
			n++;
	}
	f.close();

	n /= 2;

	ifstream f2(vertexFile.c_str());
	for(int i=0;i<n;i++){
		ScaffoldingVertex vertex;
		vertex.read(&f2);
		vertices.push_back(vertex);
	}
	f2.close();
	}

	map<uint64_t,int> lengths;

	for(int i=0;i<(int)vertices.size();i++){
		lengths[vertices[i].getName()]=vertices[i].getLength();
	}

	{
	ifstream f(edgeFile.c_str());
	int n=0;
	while(!f.eof()){
		string token="";
		f>>token;
		if(token!="")
			n++;
	}
	f.close();

	n /= 14;

	ifstream f2(edgeFile.c_str());
	for(int i=0;i<n;i++){
		ScaffoldingEdge edge;
		edge.read(&f2);
		edges.push_back(edge);
	}
	f2.close();
	}

	ScaffoldingAlgorithm solver;
	solver.setVertices(&vertices);
	solver.setEdges(&edges);

	vector<vector<uint64_t> > m_scaffoldContigs;
	vector<vector<char> >m_scaffoldStrands;
	vector<vector<int> >m_scaffoldGaps;

	solver.solve(&m_scaffoldContigs,&m_scaffoldStrands,&m_scaffoldGaps);

	for(int i=0;i<(int)m_scaffoldContigs.size();i++){
		cout<<endl;
		cout<<"Scaffold # "<<i<<endl;
		cout<<m_scaffoldContigs[i].size()<<" contigs"<<endl;
		for(int j=0;j<(int)m_scaffoldContigs[i].size();j++){
			cout<<j<<"	contig: "<<m_scaffoldContigs[i][j]<<"	strand: "<<m_scaffoldStrands[i][j]<<" length: "<<lengths[m_scaffoldContigs[i][j]]<<endl;
			if(j!=(int)m_scaffoldContigs[i].size()-1){
				cout<<"gap: "<<m_scaffoldGaps[i][j]<<endl;
			}
		}
	}

	return 0;
}
