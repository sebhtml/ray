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


#include<Chooser.h>
#include<common_functions.h>
#include<ChooserData.h>

void Chooser::chooseWithPairedReads(ExtensionData*m_ed,
	ChooserData*m_cd,
	int m_minimumCoverage,int m_maxCoverage,
	double __PAIRED_MULTIPLIER,
vector<set<int> >*battleVictories
){

	vector<int> minimumValues;
	vector<int> counts;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		int minimum=999999;
		counts.push_back(m_ed->m_EXTENSION_pairedReadPositionsForVertices[i].size());
		for(int j=0;j<(int)m_ed->m_EXTENSION_pairedReadPositionsForVertices[i].size();j++){
			int value=m_ed->m_EXTENSION_pairedReadPositionsForVertices[i][j];
			if(value<minimum){
				minimum=value;
			}
		}
		minimumValues.push_back(minimum);
	}

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if((m_cd->m_CHOOSER_theMaxsPaired[i] > __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theMaxsPaired[j])){
				(*battleVictories)[i].insert(j);
			}

			if((m_cd->m_CHOOSER_theMaxsPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theMaxsPaired[j])
			&& minimumValues[i] < 0.5 * minimumValues[j]){
				(*battleVictories)[i].insert(j);
			}
		}
	}
}

void Chooser::clear(int*a,int b){
	while(b!=0){
		a[--b]=0;
	}
}
