/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _ScaffoldingAlgorithm_h
#define _ScaffoldingAlgorithm_h

#include <vector>
#include <set>
#include <scaffolder/ScaffoldingEdge.h>
#include <map>
#include <scaffolder/ScaffoldingVertex.h>
using namespace std;

class ScaffoldingAlgorithm{
	vector<ScaffoldingVertex>*m_vertices;
	vector<ScaffoldingEdge>*m_edges;

	map<uint64_t,map<uint64_t,ScaffoldingEdge> > m_addedEdges;
	int m_numberOfEdges;

	map<uint64_t,int> m_lengths;

	map<int,map<int,int> > m_conflicts;
public:
	ScaffoldingAlgorithm();
	void setVertices(vector<ScaffoldingVertex>*vertices);
	void setEdges(vector<ScaffoldingEdge>*edges);
	void addEdge(ScaffoldingEdge*edge);

	bool hasConflictWithEdgeAroundContig(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge,uint64_t contigToCheck);
	bool hasConflictWithEdge(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge);
	bool hasConflictWithContig(ScaffoldingEdge*edge,uint64_t contig);
	bool hasConflict(ScaffoldingEdge*edge);

	void solve(

	vector<vector<uint64_t> >*m_scaffoldContigs,
	vector<vector<char> >*m_scaffoldStrands,
	vector<vector<int> >*m_scaffoldGaps
);


	void extractScaffolds(char state,map<uint64_t,int>*colors,uint64_t vertex,
	map<uint64_t,map<char,vector<vector<uint64_t> > > >*parents,
	map<uint64_t,map<char,vector<vector<uint64_t> > > >*children,set<int>*completedColours,

	vector<vector<uint64_t> >*scaffoldContigs,
	vector<vector<char> >*scaffoldStrands,
	vector<vector<int> >*scaffoldGaps
);

};

#endif
