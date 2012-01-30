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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_SeedExtender/Chooser.h>
#include <application_core/common_functions.h>

void Chooser::chooseWithPairedReads(ExtensionData*m_ed,
	double __PAIRED_MULTIPLIER,
vector<set<int> >*battleVictories,
Parameters*parameters
){

	vector<int> minimumValues;
	vector<int> counts;
	vector<int> maximumValues;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		int minimum=999999;
		int maximum=-999;
		Kmer key=m_ed->m_enumerateChoices_outgoingEdges[i];
		for(int j=0;j<(int)m_ed->m_EXTENSION_pairedReadPositionsForVertices[key].size();j++){
			int value=m_ed->m_EXTENSION_pairedReadPositionsForVertices[key][j];
			if(value>maximum){
				maximum=value;
			}
			if(value<minimum){
				minimum=value;
			}
		}

		counts.push_back(m_ed->m_EXTENSION_pairedReadPositionsForVertices[key].size());
		if(m_ed->m_EXTENSION_pairedReadPositionsForVertices[key].size()==0){
			minimum=0;
			maximum=0;
		}
		minimumValues.push_back(minimum);
		maximumValues.push_back(maximum);
	}

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		if(counts[i]==0){
			continue;
		}
		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if(maximumValues[i] > __PAIRED_MULTIPLIER*maximumValues[j]){
				(*battleVictories)[i].insert(j);
			}

			// same maximum
			if(maximumValues[i] <= __PAIRED_MULTIPLIER*maximumValues[j]
			&& maximumValues[j] <= __PAIRED_MULTIPLIER*maximumValues[i]
			&& counts[i]>20*counts[j]){
				// dodge sequencing errors.
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
