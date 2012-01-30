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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <math.h>
#include <plugin_SeedExtender/NovaEngine.h>
#include <assert.h>

/**
 * The implementation of this method is test-driven.
 * It tries to generalize a lot of cases
 */
int NovaEngine::choose(vector<map<int,int> >*distances,set<int>*invalidChoices,bool show){
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

	if(withElements==0)
		return IMPOSSIBLE_CHOICE;

	if(show){
		cout<<"Ray NovaEngine"<<endl;
		cout<<"Choices: "<<choices<<endl;
	}

	vector<int> maximumValues;
	int theMaximum=0;
	for(int i=0;i<choices;i++){
		int maximumValue=0;
		for(map<int,int>::iterator j=distances->at(i).begin();j!=distances->at(i).end();j++){
			int distance=j->first;
			if(distance>maximumValue)
				maximumValue=distance;
		}
		maximumValues.push_back(maximumValue);
		if(maximumValue>theMaximum)
			theMaximum=maximumValue;
	}

	bool allHaveAtLeast2=true;

	for(int i=0;i<choices;i++){
		int numberOfEntriesForI=distances->at(i).size();
	
		if(numberOfEntriesForI < 2)
			allHaveAtLeast2=false;
	}

	/** choose with the maximum value, if possible */
	double multiplicator=1.4;
	for(int i=0;i<choices;i++){
		bool win=true;

		int numberOfEntriesForI=distances->at(i).size();
	
		/* an invalid choice can not win */
		if(invalidChoices->count(i)>0)
			continue;

		for(int j=0;j<choices;j++){
			if(i==j)
				continue;

			int numberOfEntriesForJ=distances->at(j).size();

			/* probably a sequencing error */
			if((numberOfEntriesForJ >= numberOfEntriesForI && numberOfEntriesForI == 1)
			|| (numberOfEntriesForJ >= 2*numberOfEntriesForI && numberOfEntriesForI < 3)){
				win=false;
				break;
			}

			/* an invalid choice does not need to be tested against */
			if(invalidChoices->count(j)>0)
				continue;

			if(multiplicator*maximumValues[j] >= maximumValues[i]){
				win=false;
				break;
			}
		}
		if(win)
			return i;
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

		/** change the number of bins depending on the range of values */
		int step=32;

		if(allHaveAtLeast2)
			step=128;

		if(theMaximum> 2048 && allHaveAtLeast2)
			step=256;

		if(theMaximum>8192 && allHaveAtLeast2)
			step=512;

		for(int j=0;j<n;j++){
			int distance=observedDistances[j];
			int weight=weights[j];
			coverage[distance/step]+=weight;
		}

		for(map<int,int>::iterator j=coverage.begin();j!=coverage.end();j++){
			score++;
		}

		if(show){
			cout<<"step= "<<step<<endl;
		
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

	int selection=IMPOSSIBLE_CHOICE;
	for(int i=0;i<choices;i++){
		bool winner=true;
		
		if(invalidChoices->count(i) > 0)
			continue;

		for(int j=0;j<choices;j++){
			if(i==j)
				continue;

			if(invalidChoices->count(j)>0)
				continue;

			if(maximumValues[i]>=maximumValues[j]*100)
				continue;

			if(novaScores[j]==1 && novaScores[i]==2)
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
			cout<<"NovaEngine Selection: IMPOSSIBLE_CHOICE"<<endl;
		}else{
			cout<<"NovaEngine Selection: "<<selection+1<<endl;
		}
	}

	return selection;
}

