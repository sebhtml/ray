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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _ScaffoldingAlgorithm_h
#define _ScaffoldingAlgorithm_h

#include <vector>
#include <set>
#include <plugin_Scaffolder/ScaffoldingEdge.h>
#include <map>
#include <plugin_Scaffolder/ScaffoldingVertex.h>
using namespace std;

class ScaffoldingAlgorithm{
	vector<ScaffoldingVertex>*m_vertices;
	vector<ScaffoldingEdge>*m_edges;

	map<PathHandle,map<PathHandle,ScaffoldingEdge> > m_addedEdges;
	int m_numberOfEdges;

	map<PathHandle,int> m_lengths;

	map<int,map<int,int> > m_conflicts;
public:
	ScaffoldingAlgorithm();
	void setVertices(vector<ScaffoldingVertex>*vertices);
	void setEdges(vector<ScaffoldingEdge>*edges);
	void addEdge(ScaffoldingEdge*edge);

	bool hasConflictWithEdgeAroundContig(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge,PathHandle contigToCheck);
	bool hasConflictWithEdge(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge);
	bool hasConflictWithContig(ScaffoldingEdge*edge,PathHandle contig);
	bool hasConflict(ScaffoldingEdge*edge);

	void solve(

	vector<vector<PathHandle> >*m_scaffoldContigs,
	vector<vector<char> >*m_scaffoldStrands,
	vector<vector<int> >*m_scaffoldGaps
);


	void extractScaffolds(char state,map<PathHandle,int>*colors,PathHandle vertex,
	map<PathHandle,map<char,vector<vector<PathHandle> > > >*parents,
	map<PathHandle,map<char,vector<vector<PathHandle> > > >*children,set<int>*completedColours,

	vector<vector<PathHandle> >*scaffoldContigs,
	vector<vector<char> >*scaffoldStrands,
	vector<vector<int> >*scaffoldGaps
);

};

#endif
