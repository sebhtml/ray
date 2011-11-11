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

#ifndef _GraphImplementationDeBruijn_h
#define _GraphImplementationDeBruijn_h

#include <routing/GraphImplementation.h>
#include <vector>
using namespace std;

/**
 * de Bruijn graph
 * n must be a power of something
 */
class GraphImplementationDeBruijn : public GraphImplementation{

	int getPower(int base,int exponent);

/** convert a number to a de Bruijn vertex */
	void convertToDeBruijn(int i,int base,int digits,vector<int>*tuple);

/** get the children of a de Bruijn vertex */
	void getChildren(vector<int>*vertex,vector<vector<int> >*children,int base);

/** convert a de Bruijn vertex to base 10 */
	int convertToBase10(vector<int>*vertex,int base);

	void printVertex(vector<int>*a);
	bool isAPowerOf(int n,int base);

public:

	void makeConnections(int n);
};

#endif
