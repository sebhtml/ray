/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _ScaffoldingEdge_h
#define _ScaffoldingEdge_h

#include <stdint.h>
#include <fstream>
using namespace std;

#define LEFT_SIDE 0
#define RIGHT_SIDE 1

class ScaffoldingEdge{
	uint64_t m_leftContig;
	uint64_t m_rightContig;
	char m_leftStrand;
	char m_rightStrand;

	int m_gapSize;

	int m_count1;
	int m_average1;
	int m_standardDeviation1;

	int m_count2;
	int m_average2;
	int m_standardDeviation2;
public:
	ScaffoldingEdge(uint64_t leftContig,char leftStrand,uint64_t rightContig,char rightStrand,int gapSize,
		int average1,int count1,int standardDeviation1,
		int average2,int count2,int standardDeviation2);

	ScaffoldingEdge();

	int getGapSize();
	uint64_t getLeftContig();
	char getLeftStrand();
	uint64_t getRightContig();
	char getRightStrand();

	int getCount1();
	int getAverage1();
	int getStandardDeviation1();

	int getCount2();
	int getAverage2();
	int getStandardDeviation2();

	void read(ifstream*f);

	int getPriority()const ;
	bool operator<(const ScaffoldingEdge&other)const;

	char getStrand(uint64_t name);

	int getSide(uint64_t name);

	ScaffoldingEdge getReverseEdge();

	void print();
};

/*
bool operator<(const ScaffoldingEdge*a,const ScaffoldingEdge*b){
	return (*a)<(*b);
}
*/


#endif
