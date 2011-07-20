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

#include <math.h>
#include <heuristics/RayNovaEngine.h>
#include <assert.h>

int RayNovaEngine::choose(vector<map<int,int> >*distances,set<int>*invalidChoices,bool show){
	vector<double> novaScores;
	int choices=distances->size();

	int withElements=0;
	int initialChoice=0;
	for(int i=0;i<choices;i++){
		int n=distances->at(i).size();
		if(n>0){
			withElements++;
			initialChoice=i;
		}
	}
	
	if(withElements==1)
		return initialChoice;

	if(show){
		cout<<"Ray NovaEngine"<<endl;
		cout<<"Choices: "<<choices<<endl;
	}
	for(int i=0;i<choices;i++){
		int n=distances->at(i).size();
		vector<int> observedDistances;
		vector<int> weights;
		int totalWeight=0;
		for(map<int,int>::reverse_iterator j=distances->at(i).rbegin(); j!=distances->at(i).rend();j++){
			observedDistances.push_back(j->first);
			weights.push_back(j->second);
			totalWeight+=j->second;
		}

		double score=0;
		map<int,int> coverage;
		int step=64;
		for(int j=0;j<n;j++){
			int distance=observedDistances[j];
			int weight=weights[j];
			coverage[distance/step]+=weight;
		}

		for(map<int,int>::iterator j=coverage.begin();j!=coverage.end();j++){
			score++;
		}
/*
		for(int j=0;j<n-1;j++){
			int diff=observedDistances[j]-observedDistances[j+1];
			score+=(weights[j]+0.0)*(diff);
			cout<<"Diff: "<<diff<<" Weight: "<<weights[j]<<" CurrentScore: "<<score<<endl;
			if(diff>maxDiff)
				maxDiff=diff;
		}
		if(n>0){
			int diff=observedDistances[n-1]-0.0;
			score+=(weights[n-1]+0.0)*(diff);
			cout<<"Diff: "<<diff<<" Weight: "<<weights[n-1]<<" CurrentScore: "<<score<<endl;
			if(diff>maxDiff)
				maxDiff=diff;
			score/=(totalWeight+0.0);
		}
*/
		if(show){
			cout<<"Choice: "<<i+1<<endl;
			cout<<" DataPoints: "<<n<<endl;
			for(int j=0;j<n;j++){
				cout<<" "<<observedDistances[j]<<" "<<weights[j]<<"";
			}
			cout<<endl;
			cout<<" NovaScore: "<<score<<endl;
		}
		novaScores.push_back(score);
	}

	double threshold=0.5;
	
	int selection=IMPOSSIBLE_CHOICE;
	for(int i=0;i<choices;i++){
		bool winner=true;
		for(int j=0;j<choices;j++){
			if(i==j)
				continue;
			if(invalidChoices->count(j)>0)
				continue;
			double ratio=novaScores[j]/(novaScores[i]+0.0);
			if(novaScores[j]<3 && ratio>threshold)
				winner=false;
			if(novaScores[j]>=novaScores[i])
				winner=false;
		}
		if(winner){
			selection=i;
			break;
		}
	}
	
	if(show){
		if(selection==IMPOSSIBLE_CHOICE){
			cout<<"Selection: IMPOSSIBLE_CHOICE"<<endl;
		}else{
			cout<<"Selection: "<<selection+1<<endl;
		}
	}

	return selection;
}

