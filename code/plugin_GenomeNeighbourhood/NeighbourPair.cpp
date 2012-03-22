/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <plugin_GenomeNeighbourhood/NeighbourPair.h>

NeighbourPair::NeighbourPair(uint64_t contig1,char strand1,int progression1,uint64_t contig2,char strand2,int progression2,
		int depth){

	m_contigName1=contig1;
	m_strand1=strand1;
	m_progression1=progression1;

	m_contigName2=contig2;
	m_strand2=strand2;
	m_progression2=progression2;

	m_depth=depth;
}

char NeighbourPair::getStrand1(){
	return m_strand1;
}

uint64_t NeighbourPair::getContig1(){
	return m_contigName1;
}

int NeighbourPair::getProgression1(){
	return m_progression1;
}

char NeighbourPair::getStrand2(){
	return m_strand2;
}

uint64_t NeighbourPair::getContig2(){
	return m_contigName2;
}

int NeighbourPair::getProgression2(){
	return m_progression2;
}

int NeighbourPair::getDepth(){
	return m_depth;
}

