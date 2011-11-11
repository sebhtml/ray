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
void GraphImplementationDeBruijn::convertToDeBruijn(int i,int base,int digits,
vector<int>*tuple){
	for(int power=0;power<digits;power++){
		int value=(i%getPower(base,power+1))/getPower(base,power);
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
		m_connections.push_back(b);
	}

	m_verbose=true;

	int digits=1;

	int deBruijnGraphSize=getPower(base,digits);
	while(m_size > deBruijnGraphSize){
		digits++;
		deBruijnGraphSize=getPower(base,digits);
	}

	if(m_verbose){
		cout<<"using "<<digits<<" digits with base "<<base<<endl;
		cout<<"The MPI graph has "<<m_size<<" vertices"<<endl;
		cout<<"The de Bruijn graph has "<<deBruijnGraphSize<<" vertices"<<endl;
	}

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
		convertToDeBruijn(i,base,digits,&deBruijnVertex);

		vector<vector<int> > children;
		getChildren(&deBruijnVertex,&children,base);

		cout<<children.size()<<" children"<<endl;

		for(int j=0;j<(int)children.size();j++){

			int otherVertex=convertToBase10(&(children[j]),base);
			int rank2=otherVertex % m_size;
			
			cout<<"de Bruijn ";
			printVertex(&deBruijnVertex);
			cout<<" ("<<i<<")";
			cout<<" -> ";
			printVertex(&(children[j]));
			cout<<" ("<<otherVertex<<")";
			cout<<endl;

			// insert the self link
			m_connections[i].insert(rank2);
			cout<<"MPI "<<i<<" -> "<<rank2<<endl;
		}
	}
}

void GraphImplementationDeBruijn::getChildren(vector<int>*vertex,vector<vector<int> >*children,int base){
	for(int i=0;i<base;i++){
		vector<int> child;
		for(int j=1;j<(int)vertex->size();j++)
			child.push_back(vertex->at(j));
		child.push_back(i);

		children->push_back(child);
	}
}

int GraphImplementationDeBruijn::convertToBase10(vector<int>*vertex,int base){
	int a=0;
	int n=vertex->size();
	for(int i=0;i<n;i++){
		a+=vertex->at(i)*getPower(base,i);
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
