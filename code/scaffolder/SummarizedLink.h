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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _SummarizedLink
#define _SummarizedLink

#include <stdint.h>

class SummarizedLink{
	int m_count;
	uint64_t m_leftContig;
	uint64_t m_rightContig;
	char m_leftStrand;
	char m_rightStrand;
	int m_average;
	int m_standardDeviation;
public:
	SummarizedLink(uint64_t leftContig,char leftStrand,uint64_t rightContig,char rightStrand,int average,int count,int standardDeviation);
	SummarizedLink();
	int getStandardDeviation();
	uint64_t getLeftContig();
	char getLeftStrand();
	uint64_t getRightContig();
	char getRightStrand();
	int getAverage();
	int getCount();
	void pack(uint64_t*a,int*b);
	void unpack(uint64_t*buffer,int*position);
};

#endif
