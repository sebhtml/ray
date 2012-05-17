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

#ifndef _ContigHit_h
#define _ContigHit_h

#include <application_core/constants.h>
#include <core/types.h>
#include <stdint.h>

class ContigHit{
	int m_sequenceId;
	PathHandle m_contigId;
	Strand m_contigStrand;
	int m_matches;
public:
	ContigHit(int sequence,PathHandle contig,Strand strand,int matches);
	int getSequence();
	PathHandle getContig();
	Strand getStrand();
	int getMatches();
};

#endif
