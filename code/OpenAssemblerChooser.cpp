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

#include<OpenAssemblerChooser.h>
#include<Chooser.h>

void OpenAssemblerChooser::updateMultiplicators(int m_peakCoverage){
	m_singleEndMultiplicator=3.0;
	m_pairedEndMultiplicator=3.0;
	if(m_peakCoverage>=20){
		m_singleEndMultiplicator=m_pairedEndMultiplicator=2.0;
	}
	if(m_peakCoverage>=25){
		m_singleEndMultiplicator=m_pairedEndMultiplicator=1.3;
	}
	if(m_peakCoverage>=100){
		//m_singleEndMultiplicator=m_pairedEndMultiplicator=1.0;
	}
}

void OpenAssemblerChooser::constructor(int m_peakCoverage){
	updateMultiplicators(m_peakCoverage);
}

int OpenAssemblerChooser::choose(ExtensionData*m_ed,Chooser*m_c,int m_minimumCoverage,int m_maxCoverage,ChooserData*m_cd,
Parameters*parameters){
	vector<set<int> > battleVictories;

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		set<int> victories;
		battleVictories.push_back(victories);
	}

	m_c->chooseWithPairedReads(m_ed,m_cd,m_minimumCoverage,m_maxCoverage,m_pairedEndMultiplicator,&battleVictories,parameters);
	
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
		return pairedChoice;
	}

	// win or lose with single-end reads
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		if(m_cd->m_CHOOSER_theMaxs[i]<5){
			continue;
		}

		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if((m_cd->m_CHOOSER_theMaxs[i] > m_singleEndMultiplicator*m_cd->m_CHOOSER_theMaxs[j]) 
				&& (m_cd->m_CHOOSER_theSums[i] > m_singleEndMultiplicator*m_cd->m_CHOOSER_theSums[j]) 
				&& (m_cd->m_CHOOSER_theNumbers[i] > m_singleEndMultiplicator*m_cd->m_CHOOSER_theNumbers[j])
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

int OpenAssemblerChooser::proceedWithCoverages(int a,int b,
	ExtensionData*m_ed
){
	vector<int> counts2;
	vector<int> counts5;
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		int j2=0;
		int j5=0;
		if(m_ed->m_EXTENSION_readPositionsForVertices.count(i)>0){
			for(int k=0;k<(int)m_ed->m_EXTENSION_readPositionsForVertices[i].size();k++){
				int distanceFromOrigin=m_ed->m_EXTENSION_readPositionsForVertices[i][k];
				if(distanceFromOrigin>=2){
					j2++;
				}
				if(distanceFromOrigin>=5){
					j5++;
				}
			}
		}
		counts2.push_back(j2);
		counts5.push_back(j5);
	}

	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		bool isBetter=true;
		int coverageI=(*m_ed->m_EXTENSION_coverages)[i];
		int singleReadsI=m_ed->m_EXTENSION_readPositionsForVertices[i].size();
		if(counts2[i]==0)
			continue;

		// in less than 10% of the coverage is supported by displayed reads, abort it...
		if(singleReadsI*10 < coverageI){
			continue;
		}

		for(int j=0;j<(int)m_ed->m_enumerateChoices_outgoingEdges.size();j++){
			if(i==j)
				continue;
			//int coverageJ=m_ed->m_EXTENSION_coverages[j];
			//int singleReadsJ=m_ed->m_EXTENSION_readPositionsForVertices[j].size();
			if(!(counts2[j]<=a and counts2[i]>=b)){
				isBetter=false;
				break;
			}
		}
		if(isBetter){
			#ifdef SHOW_CHOICE
			cout<<"Choice #"<<i+1<<" wins, with "<<m_ed->m_EXTENSION_readPositionsForVertices[i].size()<<" reads."<<endl;
			cout<<" in range: "<<m_ed->m_EXTENSION_readsInRange.size()<<endl;
			#endif
			
			return i;
		}
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
