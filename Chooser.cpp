/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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


#include<Chooser.h>
#include<common_functions.h>
#include<ChooserData.h>

int Chooser::chooseWithPairedReads(
	ExtensionData*m_ed,
	ChooserData*m_cd,
	int m_minimumCoverage,int m_maxCoverage,
	double __PAIRED_MULTIPLIER
){
	// win or lose with paired reads
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		bool winner=true;
		int coverageI=m_ed->m_EXTENSION_coverages[i];
		//int singleReadsI=m_ed->m_EXTENSION_readPositionsForVertices[i].size();
		if(coverageI<_MINIMUM_COVERAGE)
			continue;
		if(m_cd->m_CHOOSER_theNumbers[i]==0 or m_cd->m_CHOOSER_theNumbersPaired[i]==0)
			continue;
		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if(i==j)
				continue;
			int coverageJ=m_ed->m_EXTENSION_coverages[j];
			if(coverageJ<_MINIMUM_COVERAGE)
				continue;
			if((m_cd->m_CHOOSER_theMaxsPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theMaxsPaired[j]) or
		 (m_cd->m_CHOOSER_theSumsPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theSumsPaired[j]) or
		 (m_cd->m_CHOOSER_theNumbersPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theNumbersPaired[j])) {

				winner=false;
				break;
			}
	
			// if the winner does not have too much coverage.
			if(m_ed->m_EXTENSION_coverages[i]<m_minimumCoverage and 
		m_cd->m_CHOOSER_theNumbers[i] < m_cd->m_CHOOSER_theNumbers[j]){// make sure that it also has more single-end reads
				winner=false;
				break;
			}
		}
		if(winner==true){
			return i;
		}
	}
	return IMPOSSIBLE_CHOICE;
}

void Chooser::clear(int*a,int b){
	while(b!=0){
		a[--b]=0;
	}
}
