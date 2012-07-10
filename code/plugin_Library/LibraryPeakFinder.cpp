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

#include <plugin_Library/LibraryPeakFinder.h>
#include <core/statistics.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#include <assert.h>
using namespace std;

#define CONFIG_SOFT_SIGNAL_THRESHOLD 32
#define CONFIG_PEAK_MINIMUM_SIGNAL 2

/** find multiple peaks in the distribution of inserts for a library */
void LibraryPeakFinder::findPeaks(vector<int>*x,vector<int>*y,vector<int>*peakAverages,vector<int>*peakStandardDeviation){

	/* the special case of simulated data
 * with a standard deviation of 0 */

	uint64_t total=0;

	for(int i=0;i<(int)y->size();i++){
		total+=y->at(i);
	}

	for(int i=0;i<(int)y->size();i++){
		if(total==0){
			break;
		}
		double ratio=y->at(i)/(total+0.0);
	
		if(y->size()<=16 && ratio>=0.90){

			peakAverages->push_back(x->at(i));
			peakStandardDeviation->push_back(i);

			cout<<"[LibraryPeakFinder] too few points"<<endl;

			return;
		}

	}

	// Now we begin the real analysis

	vector<int> backgroundData;
	
	for(int i=0;i<(int)y->size();i++){

		int verticalValue=y->at(i);

		if(verticalValue < CONFIG_PEAK_MINIMUM_SIGNAL){
			continue;
		}

		if(verticalValue < CONFIG_SOFT_SIGNAL_THRESHOLD){
			backgroundData.push_back(y->at(i));
		}
	}

	int signalAverage=(int)getAverage(&backgroundData);

	#define VERBOSE

	#ifdef VERBOSE
	int signalMode=getMode(&backgroundData);
	cout<<"Mode= "<<signalMode<<" signalAverage= "<<signalAverage<<endl;
	#endif

	#undef VERBOSE

	int signalThreshold=signalAverage;

	int minimumAccumulatedNoiseSignals=8;
	int minimumAccumulatedWorthySignals=16;
	int accumulatedNoiseSignals=0;
	int accumulatedWorthySignals=0;

	vector<int> bestHits;

	bool hasHit=false;
	int bestHit=-1;
	
	for(int i=0;i<(int)x->size();i++){
		int x1=x->at(i);
		int y1=y->at(i);

		/* skip the noise */
		if(y1 < signalThreshold){
			accumulatedNoiseSignals++;

			if(hasHit
				&& accumulatedNoiseSignals >= minimumAccumulatedNoiseSignals
			){
				cout<<"CURRENT IS NOISE, "<<x1<<endl;
				bestHits.push_back(bestHit);
				hasHit=false;
				cout<<"GOT HIT "<<x->at(bestHit)<<endl;
			}

			accumulatedWorthySignals=0;

			//cout<<"DATA	"<<x1<<"	"<<y1<<"	NOISE"<<endl;
			continue;
		}

		//cout<<"DATA	"<<x1<<"	"<<y1<<"	WORTHY"<<endl;

		/* if we don't have a hit, take this one */
		if(!hasHit && accumulatedWorthySignals >= minimumAccumulatedWorthySignals){
			accumulatedWorthySignals=0;
			hasHit=true;
			bestHit=i;
		}

		accumulatedWorthySignals++;
		accumulatedNoiseSignals=0;

		/* this is better than the old stuff, take it */

		if(hasHit && y1 > y->at(bestHit) && accumulatedWorthySignals >= minimumAccumulatedWorthySignals){
			bestHit=i;
		}
	}

	/* harvest crops */

	for(int i=0;i<(int)bestHits.size();i++){
		int index=bestHits[i];

		int left=index-1;
		
		int accumulated=0;
		while(left>=0 && accumulated<minimumAccumulatedNoiseSignals){
			if(y->at(left) < signalThreshold){
				accumulated++;
			}
			left--;
		}

		accumulated=0;
		int right=index+1;
		while(right<(int)y->size() && accumulated < minimumAccumulatedNoiseSignals){
			if(y->at(right) < signalThreshold){
				accumulated++;
			}
			right++;
		}

		vector<int> data;
		vector<int> frequencies;

		if(left<0)
			left=0;

		if(right>=(int)y->size())
			right=y->size()-1;

		for(int j=left;j<=right;j++){
			data.push_back(x->at(j));
			frequencies.push_back(y->at(j));
		}

		int average=getAverageFromFrequencies(&data,&frequencies);
		int standardDeviation=getDeviationFromFrequencies(&data,&frequencies);

		peakAverages->push_back(average);
		peakStandardDeviation->push_back(standardDeviation);
	}
}


