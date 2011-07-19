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

#include<heuristics/OpenAssemblerChooser.h>
#include<heuristics/Chooser.h>

void OpenAssemblerChooser::updateMultiplicators(int m_peakCoverage){
	m_singleEndMultiplicator=2.0;
	m_pairedEndMultiplicator=2.0;
}

void OpenAssemblerChooser::constructor(int m_peakCoverage){
	updateMultiplicators(m_peakCoverage);
}

int OpenAssemblerChooser::choose(ExtensionData*m_ed,Chooser*m_c,int m_minimumCoverage,int m_maxCoverage,
Parameters*parameters){
	/** prepare data for the NovaEngine */
	vector<map<int,int> > novaData;
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		Kmer key=m_ed->m_enumerateChoices_outgoingEdges[i];
		map<int,int> data;

		/** load single-end data */
		if(m_ed->m_EXTENSION_readPositionsForVertices.count(key)>0){
			for(vector<int>::iterator j=m_ed->m_EXTENSION_readPositionsForVertices[key].begin();
				j!=m_ed->m_EXTENSION_readPositionsForVertices[key].end();j++){
				int val=*j;
				data[val]++;
			}
		}
		/** load paired-end data and mate-pair data */
		if(m_ed->m_EXTENSION_pairedReadPositionsForVertices.count(key)>0){
			for(int j=0;j<(int)m_ed->m_EXTENSION_pairedReadPositionsForVertices[key].size();j++){
				int value=m_ed->m_EXTENSION_pairedReadPositionsForVertices[key][j];
				data[value]++;
			}
		}

		novaData.push_back(data);
	}

	/** NovaData are ready, now call the NovaEngine */
	cout<<"Calling NovaEngine.."<<endl;
	int novaChoice=m_novaEngine.choose(&novaData);

	vector<set<int> > battleVictories;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		set<int> victories;
		battleVictories.push_back(victories);
	}

	chooseWithCoverage(m_ed,m_minimumCoverage,&battleVictories);

	int coverageWinner=getWinner(&battleVictories,m_ed->m_enumerateChoices_outgoingEdges.size());
	if(coverageWinner!=IMPOSSIBLE_CHOICE)
		return coverageWinner;

	m_c->chooseWithPairedReads(m_ed,m_minimumCoverage,m_maxCoverage,m_pairedEndMultiplicator,&battleVictories,parameters);
	
	int pairedChoice=getWinner(&battleVictories,m_ed->m_enumerateChoices_outgoingEdges.size());

	battleVictories.clear();

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		set<int> victories;
		battleVictories.push_back(victories);
	}


	if(pairedChoice!=IMPOSSIBLE_CHOICE){
		#ifdef SHOW_CHOICE
		if(m_ed->m_enumerateChoices_outgoingEdges.size()>1){
			cout<<"Choice "<<pairedChoice+1<<" wins with paired-end reads."<<endl;
		}
		#endif
		if(novaChoice!=pairedChoice){
			cout<<"NovaEngine says "<<novaChoice<<" but PairedChooser says "<<pairedChoice<<endl;
		}
		return pairedChoice;
	}else{
		if(m_ed->m_EXTENSION_extension->size()>50000){
			if(!parameters->hasPairedReads()){
				return IMPOSSIBLE_CHOICE;
			}
		}
		// if both have paired reads and that is not enough for one of them to win, then abort
		int withPairedReads=0;
		
		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			Kmer key=m_ed->m_enumerateChoices_outgoingEdges[j];
			if(m_ed->m_EXTENSION_pairedReadPositionsForVertices[key].size()>0){
				withPairedReads++;
			}
		}
		if(withPairedReads!=0){
			return IMPOSSIBLE_CHOICE;
		}
	}

	map<int,int> CHOOSER_theMaxs;
	map<int,int> CHOOSER_theNumbers;
	map<int,int> CHOOSER_theSums;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		Kmer key=m_ed->m_enumerateChoices_outgoingEdges[i];
		int max=0;
		int n=0;
		int sum=0;
		if(m_ed->m_EXTENSION_readPositionsForVertices.count(key)>0){
			for(vector<int>::iterator j=m_ed->m_EXTENSION_readPositionsForVertices[key].begin();
				j!=m_ed->m_EXTENSION_readPositionsForVertices[key].end();j++){
				int val=*j;
				if(val>max){
					max=val;
				}
				n++;
				sum+=val;
			}
		}
		CHOOSER_theSums[i]=sum;
		CHOOSER_theNumbers[i]=n;
		CHOOSER_theMaxs[i]=max;
	}

	// win or lose with single-end reads
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		if(CHOOSER_theMaxs[i]<5){
			continue;
		}

		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if((CHOOSER_theMaxs[i] > m_singleEndMultiplicator*CHOOSER_theMaxs[j]) 
				&& (CHOOSER_theSums[i] > m_singleEndMultiplicator*CHOOSER_theSums[j]) 
				&& (CHOOSER_theNumbers[i] > m_singleEndMultiplicator*CHOOSER_theNumbers[j])
				){
				battleVictories[i].insert(j);
			}
		}
	}

	int finalWinner=getWinner(&battleVictories,m_ed->m_enumerateChoices_outgoingEdges.size());

	if(finalWinner!=IMPOSSIBLE_CHOICE){
		return finalWinner;
	}

	return IMPOSSIBLE_CHOICE;
}


int OpenAssemblerChooser::getWinner(vector<set<int> >*battleVictories,int choices){
	for(int winner=0;winner<(int)battleVictories->size();winner++){
		int wins=battleVictories->at(winner).size();
		if(wins+1==choices){
			return winner;
		}
	}
	return IMPOSSIBLE_CHOICE;
}

void OpenAssemblerChooser::chooseWithCoverage(ExtensionData*ed,int minCoverage,vector<set<int> >*battleVictories){
	for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
		int coverageForI=ed->m_EXTENSION_coverages->at(i);
		Kmer key=ed->m_enumerateChoices_outgoingEdges[i];

		for(int j=0;j<(int)ed->m_enumerateChoices_outgoingEdges.size();j++){
			if(i==j){
				continue;
			}
			int coverageForJ=ed->m_EXTENSION_coverages->at(j);

			if(coverageForI>=2*minCoverage && coverageForJ<=minCoverage/2){
				(*battleVictories)[i].insert(j);
			}
/*
			if(coverageForI>=minCoverage && coverageForJ<=minCoverage/2){
				(*battleVictories)[i].insert(j);
			}
*/
		}
	}
}


