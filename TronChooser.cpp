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


#include<TronChooser.h>
#include<math.h>

double score(vector<int>*a,vector<int>*b){
	int n=a->size()+b->size();
	if(n==0)
		return 0;
	double s=0;
	for(int i=0;i<(int)a->size();i++){
		int v=a->at(i);
		s+=log(v)*v;
	}
	for(int i=0;i<(int)b->size();i++){
		int v=b->at(i);
		s+=log(v)*v;
	}
	double norm=s/n;
	return norm;
}

int TronChooser::choose(ExtensionData*m_ed,Chooser*m_c,int m_minimumCoverage,int m_maxCoverage,ChooserData*m_cd){
	double scores[4];
	int i=0;
	int n=m_ed->m_enumerateChoices_outgoingEdges.size();
	while(i<n)
		scores[i++]=0;
	for(int i=0;i<n;i++){
		scores[i]=score(&(m_ed->m_EXTENSION_readPositionsForVertices[i]),
			 &(m_ed->m_EXTENSION_pairedReadPositionsForVertices[i]));
	}
	for(int i=0;i<n;i++){
		double vi=scores[i];
		if(vi==0)
			continue;
		for(int j=0;j<n;j++){
			if(i==j)
				continue;
			double vj=scores[j];
			if(vj==0)
				continue;
			double scale=vi/vj;
			double scaleLimit=2;
			if(scale<scaleLimit){
				return IMPOSSIBLE_CHOICE;
			}
		}
		return i;
	}
	return IMPOSSIBLE_CHOICE;
}
