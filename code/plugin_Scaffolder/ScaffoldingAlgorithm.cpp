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

#include <plugin_Scaffolder/ScaffoldingAlgorithm.h>
#include <queue>
#include <iostream>
#include <set>
#include <vector>
#include <assert.h>
#include <map>
using namespace std;

ScaffoldingAlgorithm::ScaffoldingAlgorithm(){
}

void ScaffoldingAlgorithm::setVertices(vector<ScaffoldingVertex>*vertices){
	m_vertices=vertices;
}

void ScaffoldingAlgorithm::setEdges(vector<ScaffoldingEdge>*edges){
	m_edges=edges;
}

/* TODO: use pointers or references to store ScaffoldingEdge objects */
void ScaffoldingAlgorithm::solve(
	vector<vector<PathHandle> >*scaffoldContigs,
	vector<vector<char> >*scaffoldStrands,
	vector<vector<int> >*scaffoldGaps
		){

	cout<<"Welcome to GreedySolver v1.0"<<endl;

	map<PathHandle,map<PathHandle,int> > validCounts;

	vector<vector<uint64_t> > megaLinks;

	cout<<m_vertices->size()<<" vertices; "<<m_edges->size()<<" edges"<<endl;

	int threshold=500;

	int large=0;
	for(int i=0;i<(int)m_vertices->size();i++){
		if(m_vertices->at(i).getLength() >= threshold){
			large++;
		}
		m_lengths[m_vertices->at(i).getName()]=m_vertices->at(i).getLength();
	}

	cout<<large<<" contigs >= "<<threshold<<endl;

	priority_queue<ScaffoldingEdge> bagOfChoices;

	for(int i=0;i<(int)m_edges->size();i++){
		bagOfChoices.push((*m_edges)[i]);
	}

	m_numberOfEdges=0;

	while(!bagOfChoices.empty()){
		ScaffoldingEdge edge=bagOfChoices.top();
		
		//cout<<"Want to add"<<endl;
		//edge.print();

		if(!hasConflict(&edge)){
			addEdge(&edge);
		}else{
			/* solve conflict */
		}

		bagOfChoices.pop();
	}

	cout<<"Edges in solution: "<<m_numberOfEdges<<endl;

	cout<<endl;

	cout<<"Conflict statistics:"<<endl;

	for(map<int,map<int,int> >::iterator i=m_conflicts.begin();i!=m_conflicts.end();i++){
		int sum=0;
		for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();j++){
			sum+=j->second;
		}
		cout<<i->first<<"	"<<sum<<endl;
	}


	/* beginning of code to move */

	/* input: ScaffoldingVertex objects and ScaffoldingEdge objects */
	/** filter edges with greedy algorithm */
	/** populate megaLinks with accepted edges */

	for(map<PathHandle,map<PathHandle,ScaffoldingEdge> >::iterator i=m_addedEdges.begin();i!=m_addedEdges.end();i++){
		for(map<PathHandle,ScaffoldingEdge>::iterator j=i->second.begin();j!=i->second.end();j++){
			PathHandle first=i->first;
			PathHandle second=j->first;

			if(second < first){
				continue;
			}

			ScaffoldingEdge edge=j->second;
		
			vector<uint64_t> megaLink;
			PathHandle leftContig=edge.getLeftContig();
			megaLink.push_back(leftContig);
			char leftStrand=edge.getLeftStrand();
			megaLink.push_back(leftStrand);
			PathHandle rightContig=edge.getRightContig();
			megaLink.push_back(rightContig);
			char rightStrand=edge.getRightStrand();
			megaLink.push_back(rightStrand);
			int average=edge.getGapSize();
			megaLink.push_back(average);
			megaLinks.push_back(megaLink);

			validCounts[leftContig][rightContig]++;
			validCounts[rightContig][leftContig]++;
		}
	}

	// create the graph
	set<PathHandle> vertices;
	map<PathHandle,map<char,vector<vector<PathHandle> > > > parents;
	map<PathHandle,map<char,vector<vector<PathHandle> > > > children;

	for(int i=0;i<(int)megaLinks.size();i++){
		PathHandle leftContig=megaLinks[i][0];
		char leftStrand=megaLinks[i][1];
		PathHandle rightContig=megaLinks[i][2];
		char rightStrand=megaLinks[i][3];
		int distance=megaLinks[i][4];
		char otherLeftStrand='F';
		vertices.insert(leftContig);
		vertices.insert(rightContig);
		if(leftStrand=='F')
			otherLeftStrand='R';
		char otherRightStrand='F';
		if(rightStrand=='F')
			otherRightStrand='R';
		children[leftContig][leftStrand].push_back(megaLinks[i]);
		parents[rightContig][rightStrand].push_back(megaLinks[i]);

		vector<uint64_t> reverseLink;
		reverseLink.push_back(rightContig);
		reverseLink.push_back(otherRightStrand);
		reverseLink.push_back(leftContig);
		reverseLink.push_back(otherLeftStrand);
		reverseLink.push_back(distance);

		children[rightContig][otherRightStrand].push_back(reverseLink);
		parents[leftContig][otherLeftStrand].push_back(reverseLink);
	}

	// add colors to the graph
	map<PathHandle,int> colors;
	map<int,vector<PathHandle> > colorMap;
	int i=0;
	for(set<PathHandle>::iterator j=vertices.begin();j!=vertices.end();j++){
		colors[*j]=i;
		colorMap[i].push_back(*j);
		i++;
	}
	
	// do some color merging.
	for(set<PathHandle>::iterator j=vertices.begin();j!=vertices.end();j++){
		PathHandle vertex=*j;
		char state='F';
		if(children.count(vertex)>0&&children[vertex].count(state)>0
			&&children[vertex][state].size()==1){
			PathHandle childVertex=children[vertex][state][0][2];
			char childState=children[vertex][state][0][3];
			if(parents[childVertex][childState].size()==1
			&&validCounts[vertex][childVertex]==1){
				int currentColor=colors[vertex];
				int childColor=colors[childVertex];
				if(currentColor!=childColor){

					for(int i=0;i<(int)colorMap[childColor].size();i++){
						PathHandle otherVertex=colorMap[childColor][i];
						colors[otherVertex]=currentColor;
						colorMap[currentColor].push_back(otherVertex);
					}
					colorMap.erase(childColor);
				}
			}
		}
		state='R';
		if(children.count(vertex)>0&&children[vertex].count(state)>0
			&&children[vertex][state].size()==1){
			PathHandle childVertex=children[vertex][state][0][2];
			char childState=children[vertex][state][0][3];
			if(parents[childVertex][childState].size()==1
			&& validCounts[vertex][childVertex]==1){
				int currentColor=colors[vertex];
				int childColor=colors[childVertex];
				if(currentColor!=childColor){
					for(int i=0;i<(int)colorMap[childColor].size();i++){
						PathHandle otherVertex=colorMap[childColor][i];
						colors[otherVertex]=currentColor;
						colorMap[currentColor].push_back(otherVertex);
					}
					colorMap.erase(childColor);
				}
			}
		}
	}

	// extract scaffolds
	set<int>completedColours;
	for(set<PathHandle>::iterator j=vertices.begin();j!=vertices.end();j++){
		PathHandle vertex=*j;
		extractScaffolds('F',&colors,vertex,&parents,&children,&completedColours,scaffoldContigs,scaffoldStrands,scaffoldGaps);
		extractScaffolds('R',&colors,vertex,&parents,&children,&completedColours,scaffoldContigs,scaffoldStrands,scaffoldGaps);
	}



	// add unscaffolded stuff.
	for(int i=0;i<(int)m_vertices->size();i++){
		PathHandle contig=m_vertices->at(i).getName();
		if(colors.count(contig)==0){
			vector<PathHandle> contigs;
			vector<char> strands;
			vector<int> gaps;
			contigs.push_back(contig);
			strands.push_back('F');
			scaffoldContigs->push_back(contigs);
			scaffoldStrands->push_back(strands);
			scaffoldGaps->push_back(gaps);
		}
	}

	/* END OF CODE TO MOVE */

	LargeCount total=0;

	int numberOfLarge=0;
	for(int i=0;i<(int)scaffoldContigs->size();i++){
		int scaffoldLength=0;
		for(int j=0;j<(int)scaffoldContigs->at(i).size();j++){
			scaffoldLength+=m_lengths[scaffoldContigs->at(i)[j]];
		}

		for(int j=0;j<(int)scaffoldGaps->at(i).size();j++){
			scaffoldLength+=scaffoldGaps->at(i)[j];
		}

		if(scaffoldLength>=threshold){
			numberOfLarge++;
		}
		total+=scaffoldLength;
	}

	cout<<scaffoldContigs->size()<<" scaffolds, "<<numberOfLarge<<" >= 500"<<endl;
	cout<<"Total: "<<total<<" nucleotides"<<endl;

}

void ScaffoldingAlgorithm::addEdge(ScaffoldingEdge*edge){
	//cout<<"Adding edge "<<endl;
	//edge->print();
	PathHandle leftContig=edge->getLeftContig();
	PathHandle rightContig=edge->getRightContig();

	m_addedEdges[leftContig][rightContig]=*edge;
	m_addedEdges[rightContig][leftContig]=*edge;
	m_numberOfEdges++;
}

bool ScaffoldingAlgorithm::hasConflict(ScaffoldingEdge*edge){
	PathHandle leftContig=edge->getLeftContig();
	PathHandle rightContig=edge->getRightContig();

	if(hasConflictWithContig(edge,leftContig))
		return true;

	if(hasConflictWithContig(edge,rightContig))
		return true;

	return false;

}

bool ScaffoldingAlgorithm::hasConflictWithContig(ScaffoldingEdge*edge,PathHandle contig){
	/* the contig is now connected */
	if(m_addedEdges.count(contig) == 0)
		return false;

	/* check if there is a conflit between existing edges */
	for(map<PathHandle,ScaffoldingEdge>::iterator i=m_addedEdges[contig].begin();
			i!=m_addedEdges[contig].end();i++){
		ScaffoldingEdge otherEdge=i->second;
		
		if(hasConflictWithEdge(edge,&otherEdge))
			return true;
	}

	return false;
}

bool ScaffoldingAlgorithm::hasConflictWithEdge(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge){
	PathHandle edge1LeftContig=edgeToBeAdded->getLeftContig();
	PathHandle edge1RightContig=edgeToBeAdded->getRightContig();
	PathHandle edge2LeftContig=alreadyAcceptedEdge->getLeftContig();
	PathHandle edge2RightContig=alreadyAcceptedEdge->getRightContig();

	if(edge1LeftContig == edge2LeftContig){
		if(hasConflictWithEdgeAroundContig(edgeToBeAdded,alreadyAcceptedEdge,edge1LeftContig)){
			return true;
		}
	}

	if(edge1LeftContig == edge2RightContig){
		if(hasConflictWithEdgeAroundContig(edgeToBeAdded,alreadyAcceptedEdge,edge1LeftContig)){
			return true;
		}
	}

	if(edge1RightContig== edge2LeftContig){
		if(hasConflictWithEdgeAroundContig(edgeToBeAdded,alreadyAcceptedEdge,edge1RightContig)){
			return true;
		}
	}

	if(edge1RightContig== edge2RightContig){
		if(hasConflictWithEdgeAroundContig(edgeToBeAdded,alreadyAcceptedEdge,edge1RightContig)){
			return true;
		}
	}

	return false;
}

bool ScaffoldingAlgorithm::hasConflictWithEdgeAroundContig(ScaffoldingEdge*edgeToBeAdded,ScaffoldingEdge*alreadyAcceptedEdge,PathHandle contigToCheck){

	int edge1ContigSide=edgeToBeAdded->getSide(contigToCheck);
	char edge1ContigStrand=edgeToBeAdded->getStrand(contigToCheck);

	int edge2ContigSide=alreadyAcceptedEdge->getSide(contigToCheck);
	char edge2ContigStrand=alreadyAcceptedEdge->getStrand(contigToCheck);

	if(edge1ContigStrand == edge2ContigStrand){
		if(edge1ContigSide == edge2ContigSide){
			cout<<"Conflict for contig "<<contigToCheck<<" Length= "<<m_lengths[contigToCheck]<<endl;
			cout<<"Edge to be added:"<<endl;
			edgeToBeAdded->print();
			cout<<"Already added edge:"<<endl;
			alreadyAcceptedEdge->print();
			m_conflicts[edgeToBeAdded->getPriority()][alreadyAcceptedEdge->getPriority()]++;
			return true;
		}
	}else{
		ScaffoldingEdge reverseEdge=alreadyAcceptedEdge->getReverseEdge();

		int edge2ContigSideReverse=reverseEdge.getSide(contigToCheck);
	
		#ifdef ASSERT
		char edge2ContigStrandReverse=reverseEdge.getStrand(contigToCheck);
		assert(edge2ContigStrandReverse == edge1ContigStrand);
		#endif

		if(edge1ContigSide == edge2ContigSideReverse){
			cout<<"Conflict (reverse) for contig "<<contigToCheck<<" Length= "<<m_lengths[contigToCheck]<<endl;
			cout<<"Edge to be added:"<<endl;
			edgeToBeAdded->print();
			cout<<"Already added edge:"<<endl;
			alreadyAcceptedEdge->print();
			m_conflicts[edgeToBeAdded->getPriority()][alreadyAcceptedEdge->getPriority()]++;
			return true;
		}
	}

	return false;
}

void ScaffoldingAlgorithm::extractScaffolds(char state,map<PathHandle,int>*colors,PathHandle vertex,
	map<PathHandle,map<char,vector<vector<PathHandle> > > >*parents,
	map<PathHandle,map<char,vector<vector<PathHandle> > > >*children,set<int>*completedColours,

	vector<vector<PathHandle> >*scaffoldContigs,
	vector<vector<char> >*scaffoldStrands,
	vector<vector<int> >*scaffoldGaps
){
	vector<PathHandle> contigs;
	vector<char> strands;
	vector<int> gaps;
	bool skip=false;
	int currentColor=(*colors)[vertex];
	if((*completedColours).count(currentColor)>0)
		return;

	if((*parents).count(vertex)>0&&(*parents)[vertex].count(state)>0){
		for(int i=0;i<(int)(*parents)[vertex][state].size();i++){
			#ifdef ASSERT
			assert(0<(*parents)[vertex][state][i].size());
			#endif
			PathHandle parent=(*parents)[vertex][state][i][0];
			int parentColor=(*colors)[parent];
			if(parentColor==currentColor){
				skip=true;
				break;
			}
		}
	}
	if(skip)
		return;

	(*completedColours).insert(currentColor);
	bool done=false;
	while(!done){
		contigs.push_back(vertex);
		strands.push_back(state);
		if((*children).count(vertex)>0&&(*children)[vertex].count(state)>0){
			bool found=false;
			for(int i=0;i<(int)(*children)[vertex][state].size();i++){
				#ifdef ASSERT
				assert(2<(*children)[vertex][state][i].size());
				#endif

				PathHandle childVertex=(*children)[vertex][state][i][2];
				int childColor=(*colors)[childVertex];
				if(childColor==currentColor){
					#ifdef ASSERT
					assert(3<(*children)[vertex][state][i].size());
					#endif

					char childState=(*children)[vertex][state][i][3];

					#ifdef ASSERT
					assert(4<(*children)[vertex][state][i].size());
					#endif

					int gap=(*children)[vertex][state][i][4];
					gaps.push_back(gap);

					vertex=childVertex;
					state=childState;
					found=true;
					break;
				}
			}
			if(!found){
				done=true;
			}
		}else{
			done=true;
		}
	}
	scaffoldContigs->push_back(contigs);
	scaffoldStrands->push_back(strands);
	scaffoldGaps->push_back(gaps);
	return;
}


