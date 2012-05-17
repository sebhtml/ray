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

#include <plugin_Searcher/ContigHit.h>


ContigHit::ContigHit(int sequence,PathHandle contig,char strand,int matches){
	m_sequenceId=sequence;
	m_contigId=contig;
	m_contigStrand=strand;
	m_matches=matches;
}

int ContigHit::getSequence(){
	return m_sequenceId;
}

PathHandle ContigHit::getContig(){
	return m_contigId;
}

char ContigHit::getStrand(){
	return m_contigStrand;
}

int ContigHit::getMatches(){
	return m_matches;
}
