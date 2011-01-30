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
vector<set<int> >*battleVictories,
Parameters*parameters
){

	vector<int> minimumValues;
	vector<int> counts;
	vector<int> maximumValues;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		map<int,vector<int> > classifiedValues;
		
		for(int j=0;j<(int)m_ed->m_EXTENSION_pairedReadPositionsForVertices[i].size();j++){
			int value=m_ed->m_EXTENSION_pairedReadPositionsForVertices[i][j];
			int library=m_ed->m_EXTENSION_pairedLibrariesForVertices[i][j];
			classifiedValues[library].push_back(value);
		}

		vector<int> acceptedValues;

		int minimum=999999;
		int maximum=-999;
		for(map<int,vector<int> >::iterator j=classifiedValues.begin();j!=classifiedValues.end();j++){
			int averageLength=parameters->getLibraryAverageLength(j->first);
			int stddev=parameters->getLibraryStandardDeviation(j->first);
			int leftThreshold=averageLength-stddev/3;
			int rightThreshold=averageLength+stddev/3;
			bool hasMin=false;
			bool hasMax=false;
			int localMin=99999;
			int localMax=-999;
			for(int k=0;k<(int)j->second.size();k++){
				int val=j->second[k];
				//cout<<"Val="<<val<<" Average="<<averageLength<<endl;
				if(val<localMin){
					localMin=val;
				}
				if(val>localMax){
					localMax=val;
				}
				if(val>=leftThreshold){
					hasMax=true;
				}
				if(val<=rightThreshold){
					hasMin=true;
				}
			}
			if(hasMax&&hasMax){
				if(localMin<minimum){
					minimum=localMin;
				}
				if(localMax>maximum){
					maximum=localMax;
				}
				for(int k=0;k<(int)j->second.size();k++){
					int val=j->second[k];
					acceptedValues.push_back(val);
				}
			}
		}

		counts.push_back(acceptedValues.size());
		if(acceptedValues.size()==0){
			minimum=0;
			maximum=0;
		}
		minimumValues.push_back(minimum);
		maximumValues.push_back(maximum);
		//cout<<"CHoice="<<i<<" "<<"Max="<<maximum<<" Min="<<minimum<<endl;
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
