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

#include <routing/GraphImplementationDeBruijn.h>
#include <iostream>
using namespace std;

int GraphImplementationDeBruijn::getPower(int base,int exponent){
	int a=1;
	while(exponent--)
		a*=base;
	return a;
}

/**
 * 
 */
void GraphImplementationDeBruijn::convertToDeBruijn(int i,vector<int>*tuple){
	for(int power=0;power<m_digits;power++){
		int value=(i%getPower(m_base,power+1))/getPower(m_base,power);
		tuple->push_back(value);
	}
}

bool GraphImplementationDeBruijn::isAPowerOf(int n,int base){
	int remaining=n;
	
	while(remaining>1){
		if(remaining%base != 0)
			return false;
		remaining/=base;
	}
	return true;
}

void GraphImplementationDeBruijn::makeConnections(int n){

	int base=-1;

	int maxBase=32;

	while(maxBase>=n)
		maxBase/=2;

	for(int i=maxBase;i>=2;i--){
		if(isAPowerOf(n,i)){
			base=i;
			break;
		}
	}

	if(base==-1)
		cout<<"Error, "<<n<<" is not a power of anything."<<endl;

	m_size=n;

	for(int i=0;i<m_size;i++){
		set<Rank> b;
		m_outcomingConnections.push_back(b);
		m_incomingConnections.push_back(b);
	}

	m_verbose=true;

	int digits=1;

	int deBruijnGraphSize=getPower(base,digits);
	while(m_size > deBruijnGraphSize){
		digits++;
		deBruijnGraphSize=getPower(base,digits);
	}

	if(m_verbose){
		cout<<"[GraphImplementationDeBruijn::makeConnections] using "<<digits<<" digits with base "<<base<<endl;
		cout<<"[GraphImplementationDeBruijn::makeConnections] The MPI graph has "<<m_size<<" vertices"<<endl;
		cout<<"[GraphImplementationDeBruijn::makeConnections] The de Bruijn graph has "<<deBruijnGraphSize<<" vertices"<<endl;
	}

	m_base=base;
	m_digits=digits;

	//
	// example:
	// the current vertex is (0,1,0)
	// according to Mr de Bruijn, the 16 children are
	// (1,0,0)
	// (1,0,1)
	// ...
	// (1,0,14)
	// (1,0,15)
	//
	// we need to map vertices from one graph to another
	//
	// *******************
	// ** from de Bruijn base <base> to base 10
	//
	// for a de Bruijn graph vertex (i,j,k), its base 10 value is
	// v_bruijn = k*base^0 + j*base^1 + i*base^2
	//
	// we basically need special objects for this numbers in base <base>
	//
	// *******************
	// ** from de Bruijn base 10 to MPI base 10
	//
	// its corresponding vertex in the MPI graph is simply
	// v_bruijn % m_size
	//
	// ** from MPI base 10 to de Bruijn base 10
	//
	// it is the same because de Bruijn contains the other.
	//
	// ** de Bruijn base 10 to de Bruijn base <base>
	//
	//
	for(Rank i=0;i<m_size;i++){
		vector<int> deBruijnVertex;
		convertToDeBruijn(i,&deBruijnVertex);

		vector<vector<int> > children;
		getChildren(&deBruijnVertex,&children);

		//cout<<children.size()<<" children"<<endl;

		for(int j=0;j<(int)children.size();j++){

			int otherVertex=convertToBase10(&(children[j]));
			int rank2=otherVertex % m_size;
			
/*
			cout<<"de Bruijn ";
			printVertex(&deBruijnVertex);
			cout<<" ("<<i<<")";
			cout<<" -> ";
			printVertex(&(children[j]));
			cout<<" ("<<otherVertex<<")";
			cout<<endl;
*/

			m_outcomingConnections[i].insert(rank2);

			//cout<<"MPI "<<i<<" -> "<<rank2<<endl;
		}

		vector<vector<int> > parents;
		getParents(&deBruijnVertex,&parents);

		for(int j=0;j<(int)parents.size();j++){

			int otherVertex=convertToBase10(&(parents[j]));
			int rank2=otherVertex % m_size;
			
			m_incomingConnections[i].insert(rank2);
		}
	}
}

void GraphImplementationDeBruijn::getChildren(vector<int>*vertex,vector<vector<int> >*children){
	for(int i=0;i<m_base;i++){
		vector<int> child;
		for(int j=1;j<m_digits;j++)
			child.push_back(vertex->at(j));
		child.push_back(i);

		children->push_back(child);
	}
}

void GraphImplementationDeBruijn::getParents(vector<int>*vertex,vector<vector<int> >*parents){
	for(int i=0;i<m_base;i++){
		vector<int> parent;

		parent.push_back(i);
		for(int j=0;j<m_digits-1;j++)
			parent.push_back(vertex->at(j));

		parents->push_back(parent);
	}
}

int GraphImplementationDeBruijn::convertToBase10(vector<int>*vertex){
	int a=0;
	int n=vertex->size();
	for(int i=0;i<n;i++){
		a+=vertex->at(i)*getPower(m_base,i);
	}
	return a;
}

void GraphImplementationDeBruijn::printVertex(vector<int>*a){
	for(int i=0;i<(int)a->size();i++){
		if(i!=0)
			cout<<",";
		cout<<a->at(i);
	}
}

void GraphImplementationDeBruijn::computeRoute(Rank a,Rank b,vector<Rank>*route){
	/* do nothing because this is not utilised */
}

void GraphImplementationDeBruijn::makeRoutes(){
	/* we don't compute any routes */
	
	computeRelayEvents();
}

Rank GraphImplementationDeBruijn::getNextRankInRoute(Rank source,Rank destination,Rank current){
	/* use de Bruijn property */
	vector<int> sourceVertex;
	vector<int> destinationVertex;
	vector<int> currentVertex;

	convertToDeBruijn(source,&sourceVertex);
	convertToDeBruijn(destination,&destinationVertex);
	convertToDeBruijn(current,&currentVertex);

	// example:
	//
	// base = 16
	// digits = 3
	//
	// source = (0,4,2)
	// destination = (9,8,7)
	//
	// the path is
	//
	// (0,4,2) -> (4,2,9) -> (2,9,8) -> (9,8,7)
	//
	// so for sure we have to shift the current by one on the left
	//
	// then the problem is how to choose which symbol to add 
	//
	// let's say that
	//
	// current = (4,2,9)
	//
	// then we should return (2,9,8) rapidly
	//
	// source=(0,2,2)
	// destination=(2,2,1)
	//
	// (0,2,2) -> (2,2,1)	
	
	// do a left shift
	vector<int> next;
	for(int i=1;i<(int)currentVertex.size();i++){
		next.push_back(currentVertex[i]);
	}

	// here we need to choose a digit from the destination
	// and append it to the next
	// case 1 destination can be obtained with 1 shift, overlap is 2
	// case 2 destination can be obtained with 2 shifts, overlap is 1
	// case 3 ...
	// case m_digits destination can be obtained with m_digits shifts, overlap is 0
	
	int overlapSize=getMaximumOverlap(&currentVertex,&destinationVertex);

	// append the digit
	next.push_back(destinationVertex[overlapSize]);
	
	return convertToBase10(&next);
}

int GraphImplementationDeBruijn::getMaximumOverlap(vector<int>*a,vector<int>*b){
	int n=a->size();

	// we don't verify if they are exact matches
	// because if it would be the case, nothing would
	// need to be routed anywhere
	int numberOfMatches=n-1;

	while(1){
		// check for numberOfMatches
		int positionInA=n-numberOfMatches;
		int positionInB=0;

		bool match=true;

		while(positionInA<n){
			if(a->at(positionInA) != b->at(positionInB)){
				match=false;
				break;
			}
			positionInA++;
			positionInB++;
		}

		if(match)
			return numberOfMatches;

		numberOfMatches--;
	}

	return 0; /* will never be reached */
}
