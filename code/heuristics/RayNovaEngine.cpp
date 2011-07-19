/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include <heuristics/RayNovaEngine.h>
#include <heuristics/Chooser.h>
#include <assert.h>

int RayNovaEngine::choose(vector<map<int,int> >*distances){
	cout<<"Ray NovaEngine"<<endl;
	vector<double> novaScores;
	int choices=distances->size();

	#ifdef ASSERT
	assert(choices>1);
	#endif

	for(int i=0;i<choices;i++){
		int n=distances->at(i).size();
		vector<int> observedDistances;
		vector<int> weights;
		for(map<int,int>::reverse_iterator j=distances->at(i).rbegin(); j!=distances->at(i).rend();j++){
			observedDistances.push_back(j->first);
			weights.push_back(j->second);
		}

		double score=0;
		for(int j=0;j<n-1;j++){
			score+=(weights[j]*observedDistances[j]+0.0)/(observedDistances[j]-observedDistances[j+1]);
		}
		if(n>0)
			score+=weights[n-1]*observedDistances[n-1]/(observedDistances[n-1]+0.0);
		cout<<"Choice: "<<i+1<<"/"<<choices<<" Score: "<<score<<endl;
		cout<<" Data:";
		for(int j=0;j<n;j++){
			cout<<" ("<<observedDistances[j]<<";"<<weights[j]<<")";
		}
		cout<<endl;
	}

	return IMPOSSIBLE_CHOICE;
}

