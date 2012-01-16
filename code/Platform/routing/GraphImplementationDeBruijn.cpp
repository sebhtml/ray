/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

//#define CONFIG_ROUTING_DE_BRUIJN_COMPUTE_ROUTES

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
void GraphImplementationDeBruijn::convertToDeBruijn(int i,Tuple*tuple){
	for(int power=0;power<m_diameter;power++){
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

void GraphImplementationDeBruijn::configureGraph(int n){
	int base=-1;

	int maxBase=32;


	// use the user-provided degree, if any
	if(isAPowerOf(n,m_degree)){
		maxBase=m_degree;
	}

	while(maxBase>=n)
		maxBase/=2;

	for(int i=maxBase;i>=2;i--){
		if(isAPowerOf(n,i)){
			base=i;
			break;
		}
	}

	if(base==-1){
		cout<<"Error, "<<n<<" is not a power of anything."<<endl;
		m_base=base;
		return;
	}

	int digits=1;

	while(n > getPower(base,digits)){
		digits++;
	}

	m_base=base;
	m_diameter=digits;
	m_size=n;
}

void GraphImplementationDeBruijn::makeConnections(int n){

	configureGraph(n);

	if(m_verbose){
		cout<<"[GraphImplementationDeBruijn::makeConnections] using "<<m_diameter<<" for diameter with base ";
		cout<<m_base<<endl;
		cout<<"[GraphImplementationDeBruijn::makeConnections] The MPI graph has "<<m_size<<" vertices"<<endl;
		cout<<"[GraphImplementationDeBruijn::makeConnections] The de Bruijn graph has "<<m_size<<" vertices"<<endl;
	}

	for(Rank i=0;i<m_size;i++){
		set<Rank> b;
		m_outcomingConnections.push_back(b);
		m_incomingConnections.push_back(b);
	}

	// populate vertices
	for(Rank i=0;i<m_size;i++){
		Tuple a;
		convertToDeBruijn(i,&a);
		m_graphToDeBruijn.push_back(a);
	}

	// make all connections.
	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			if(computeConnection(i,j)){
				m_outcomingConnections[i].insert(j);
				m_incomingConnections[j].insert(i);
			}
		}
	}
}

/* base m_base to base 10 */
int GraphImplementationDeBruijn::convertToBase10(Tuple*vertex){
	int a=0;
	for(int i=0;i<m_diameter;i++){
		a+=vertex->m_digits[i]*getPower(m_base,i);
	}
	return a;
}

void GraphImplementationDeBruijn::printVertex(Tuple*a){
	for(int i=0;i<m_diameter;i++){
		if(i!=0)
			cout<<",";
		cout<<a->m_digits[i];
	}
}

/** with de Bruijn routing, no route are pre-computed at all */
void GraphImplementationDeBruijn::computeRoute(Rank source,Rank destination,vector<Rank>*route){
	/* do nothing because this is not utilised */

	Rank currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=computeNextRankInRoute(source,destination,currentVertex);
		route->push_back(currentVertex);
	}
}

Rank GraphImplementationDeBruijn::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	#ifdef CONFIG_ROUTING_DE_BRUIJN_COMPUTE_ROUTES

	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];

	#else /* compute it right away */

	return computeNextRankInRoute(source,destination,rank);

	#endif
}

/** with de Bruijn routing, no route are pre-computed at all */
void GraphImplementationDeBruijn::makeRoutes(){
	/* we don't compute any routes */
	
	#ifdef CONFIG_ROUTING_DE_BRUIJN_COMPUTE_ROUTES
	computeRoutes();
	#endif

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
 *
 *	// example:
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

	// here we need to choose a digit from the destination
	// and append it to the next
	// case 1 destination can be obtained with 1 shift, overlap is 2
	// case 2 destination can be obtained with 2 shifts, overlap is 1
	// case 3 ...
	// case m_digits destination can be obtained with m_digits shifts, overlap is 0

 */
Rank GraphImplementationDeBruijn::computeNextRankInRoute(Rank source,Rank destination,Rank current){

	Tuple*destinationVertex=&(m_graphToDeBruijn[destination]);
	Tuple*currentVertex=&(m_graphToDeBruijn[current]);
	
	// do a left shift
	Tuple next;
	for(int i=1;i<m_diameter;i++){
		next.m_digits[i-1]=currentVertex->m_digits[i];
	}
	
	int overlapSize=getMaximumOverlap(currentVertex,destinationVertex);

	// append the digit
	next.m_digits[m_diameter-1]=destinationVertex->m_digits[overlapSize];
	
	int nextRank=convertToBase10(&next) % m_size;

	return nextRank;
}

/**
 * here we find the maximum overlap between
 * 2 de Bruijn vertices
 *
 * we don't look for a perfect match
 */
int GraphImplementationDeBruijn::getMaximumOverlap(Tuple*a,Tuple*b){

	// we don't verify if they are exact matches
	// because if it would be the case, nothing would
	// need to be routed anywhere
	int numberOfMatches=m_diameter-1;

	while(1){
		// check for numberOfMatches
		int positionInA=m_diameter-numberOfMatches;
		int positionInB=0;

		bool match=true;

		while(positionInA<m_diameter){
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

bool GraphImplementationDeBruijn::isConnected(Rank source,Rank destination){
	if(source==destination)
		return true;

	return m_outcomingConnections[source].count(destination)==1;
}

/** just verify the de Bruijn property
 * also, we allow any vertex to communicate with itself
 * regardless of the de Bruijn property
 */
bool GraphImplementationDeBruijn::computeConnection(Rank source,Rank destination){

	// otherwise, we look for the de Bruijn property
	Tuple*sourceVertex=&(m_graphToDeBruijn[source]);
	Tuple*destinationVertex=&(m_graphToDeBruijn[destination]);

	int overlap=getMaximumOverlap(sourceVertex,destinationVertex);

	return overlap==m_diameter-1;
}

bool GraphImplementationDeBruijn::isValid(int n){
	configureGraph(n);

	return m_base!=-1;
}

void GraphImplementationDeBruijn::setDegree(int degree){
	m_degree=degree;
}
