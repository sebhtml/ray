/*
 	Ray
    Copyright (C) 2011, 2012  Sébastien Boisvert

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

#include "plugin_KmerAcademyBuilder/KmerCandidate.h"

Kmer KmerCandidate::getKey(){
	return m_lowerKey;
}

void KmerCandidate::setKey(Kmer key){
	m_lowerKey=key;
}

void KmerCandidate::setCoverage(Kmer*a,CoverageDepth coverage){
	if(*a==m_lowerKey){

		CoverageDepth max=0;
		max=max-1;// underflow.

		if(m_count==max){ // maximum value
			return;
		}

		m_count=coverage;
	}
}

CoverageDepth KmerCandidate::getCoverage(Kmer*a){
	return m_count;
}


