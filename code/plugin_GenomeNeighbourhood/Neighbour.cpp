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

#include <plugin_GenomeNeighbourhood/Neighbour.h>

Neighbour::Neighbour(Strand dnaStrand,int depth,PathHandle contig,int progression){
	m_strand=dnaStrand;
	m_depth=depth;
	m_contigName=contig;
	m_progression=progression;
}

Strand Neighbour::getStrand(){
	return m_strand;
}

int Neighbour::getDepth(){
	return m_depth;
}

PathHandle Neighbour::getContig(){
	return m_contigName;
}

int Neighbour::getProgression(){
	return m_progression;
}

