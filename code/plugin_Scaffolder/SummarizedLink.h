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
#include <application_core/constants.h>
#include <core/types.h>

class SummarizedLink{
	int m_count;
	PathHandle m_leftContig;
	PathHandle m_rightContig;
	Strand m_leftStrand;
	Strand m_rightStrand;
	int m_average;
	int m_standardDeviation;
public:
	SummarizedLink(PathHandle leftContig,char leftStrand,PathHandle rightContig,Strand rightStrand,int average,int count,int standardDeviation);
	SummarizedLink();
	int getStandardDeviation();
	PathHandle getLeftContig();
	Strand getLeftStrand();
	PathHandle getRightContig();
	Strand getRightStrand();
	int getAverage();
	int getCount();
	void pack(MessageUnit*a,int*b);
	void unpack(MessageUnit*buffer,int*position);
};

#endif
