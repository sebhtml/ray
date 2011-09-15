#include <scaffolder/ScaffoldingAlgorithm.h>
#include <scaffolder/ScaffoldingVertex.h>
#include <scaffolder/ScaffoldingEdge.h>
#include <string>
#include <iostream>
using namespace std;

int main(int argc,char**argv){
	string vertexFile=argv[1];
	string edgeFile=argv[2];

	vector<ScaffoldingVertex> vertices;
	vector<ScaffoldingEdge> edges;

	{
	ifstream f(vertexFile.c_str());
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

	n /= 12;

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

	return 0;
}
