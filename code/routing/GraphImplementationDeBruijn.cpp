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
 *  convert a number to a de Bruijn vertex
 */
void GraphImplementationDeBruijn::convertToDeBruijn(int i,DeBruijnVertex*tuple){
	for(int power=0;power<m_digits;power++){
		int value=(i%getPower(m_base,power+1))/getPower(m_base,power);
		tuple->m_digits[power]=value;
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
		DeBruijnVertex deBruijnVertex;
		convertToDeBruijn(i,&deBruijnVertex);

		vector<DeBruijnVertex> children;
		getChildren(&deBruijnVertex,&children);

		//cout<<children.size()<<" children"<<endl;

		for(int j=0;j<m_base;j++){

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

		vector<DeBruijnVertex> parents;
		getParents(&deBruijnVertex,&parents);

		for(int j=0;j<m_base;j++){

			int otherVertex=convertToBase10(&(parents[j]));
			int rank2=otherVertex % m_size;
			
			m_incomingConnections[i].insert(rank2);
		}
	}
}

/** shift the vertex 1 time on the left
 * and for i from 0 to m_base-1 add i at the end
 */
void GraphImplementationDeBruijn::getChildren(DeBruijnVertex*vertex,vector<DeBruijnVertex>*children){
	for(int i=0;i<m_base;i++){
		DeBruijnVertex child;
		for(int j=1;j<m_digits;j++)
			child.m_digits[j-1]=vertex->m_digits[j];
		child.m_digits[m_digits-1]=i;

		children->push_back(child);
	}
}

/** shift ou the right and
 * for i from 0 to m_base-1 add i at the beginning
 */
void GraphImplementationDeBruijn::getParents(DeBruijnVertex*vertex,vector<DeBruijnVertex>*parents){
	for(int i=0;i<m_base;i++){
		DeBruijnVertex parent;

		parent.m_digits[0]=i;
		for(int j=0;j<m_digits-1;j++)
			parent.m_digits[j+1]=vertex->m_digits[j];

		parents->push_back(parent);
	}
}

/* base m_base to base 10 */
int GraphImplementationDeBruijn::convertToBase10(DeBruijnVertex*vertex){
	int a=0;
	for(int i=0;i<m_digits;i++){
		a+=vertex->m_digits[i]*getPower(m_base,i);
	}
	return a;
}

void GraphImplementationDeBruijn::printVertex(DeBruijnVertex*a){
	for(int i=0;i<m_digits;i++){
		if(i!=0)
			cout<<",";
		cout<<a->m_digits[i];
	}
}

/** with de Bruijn routing, no route are pre-computed at all */
void GraphImplementationDeBruijn::computeRoute(Rank a,Rank b,vector<Rank>*route){
	/* do nothing because this is not utilised */
}

/** with de Bruijn routing, no route are pre-computed at all */
void GraphImplementationDeBruijn::makeRoutes(){
	/* we don't compute any routes */
	
	/* compute relay points */
	computeRelayEvents();
}

/** to get the next rank,
 * we need to shift the current one time on the left
 * then, we find the maximum overlap
 * between the current and the destination
 *
 * This value is the index of the digit in destination
 * that we want to append to the next rank in the route
 */
Rank GraphImplementationDeBruijn::getNextRankInRoute(Rank source,Rank destination,Rank current){
	/* use de Bruijn property */
	DeBruijnVertex destinationVertex;
	DeBruijnVertex currentVertex;

	// we don't need to convert source

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
	DeBruijnVertex next;
	for(int i=1;i<m_digits;i++){
		next.m_digits[i-1]=currentVertex.m_digits[i];
	}

	// here we need to choose a digit from the destination
	// and append it to the next
	// case 1 destination can be obtained with 1 shift, overlap is 2
	// case 2 destination can be obtained with 2 shifts, overlap is 1
	// case 3 ...
	// case m_digits destination can be obtained with m_digits shifts, overlap is 0
	
	int overlapSize=getMaximumOverlap(&currentVertex,&destinationVertex);

	// append the digit
	next.m_digits[m_digits-1]=destinationVertex.m_digits[overlapSize];
	
	int nextRank=convertToBase10(&next) % m_size;

	return nextRank;
}

/**
 * here we find the maximum overlap between
 * 2 de Bruijn vertices
 *
 * we don't look for a perfect match
 */
int GraphImplementationDeBruijn::getMaximumOverlap(DeBruijnVertex*a,DeBruijnVertex*b){

	// we don't verify if they are exact matches
	// because if it would be the case, nothing would
	// need to be routed anywhere
	int numberOfMatches=m_digits-1;

	while(1){
		// check for numberOfMatches
		int positionInA=m_digits-numberOfMatches;
		int positionInB=0;

		bool match=true;

		while(positionInA<m_digits){
			if(a->m_digits[positionInA] != b->m_digits[positionInB]){
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

/** just verify the de Bruijn property
 * also, we allow any vertex to communicate with itself
 * regardless of the de Bruijn property
 */
bool GraphImplementationDeBruijn::isConnected(Rank source,Rank destination){
	// communicating with itself is always allowed
	if(source==destination)
		return true;

	// otherwise, we look for the de Bruijn property
	DeBruijnVertex sourceVertex;
	convertToDeBruijn(source,&sourceVertex);

	DeBruijnVertex destinationVertex;
	convertToDeBruijn(destination,&destinationVertex);

	int overlap=getMaximumOverlap(&sourceVertex,&destinationVertex);

	return overlap==m_digits-1;
}


