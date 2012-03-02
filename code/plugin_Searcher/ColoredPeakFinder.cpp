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

#include <plugin_Searcher/ColoredPeakFinder.h>
#include <core/statistics.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#include <assert.h>
using namespace std;

#define CONFIG_SOFT_SIGNAL_THRESHOLD 32

/*
 * this algorithm is really simple
 * it only works with smooth data however 
 */
void ColoredPeakFinder::findObviousPeak(vector<int>*x,vector<int>*y,vector<int>*peakAverages,vector<int>*peakStandardDeviation){

	// just find a point for which everything on the left is increasing and everything on the right is increasing
	
	int minimumLeft=2;
	int minimumRight=8;

	if((int)y->size() < minimumLeft+ minimumRight + 1)
		return;


	for(int center=1;center<(int)y->size();center++){
		int left=0;
		int right=0;

		//cout<<"Trying center at x= "<<x->at(center)<<endl;

		int currentLeft=center-1;

		while(currentLeft>=0){
			if(y->at(currentLeft) >= y->at(currentLeft+1)){
				break; // not increasing
			}

			left++;
			currentLeft--;
		}

		// this center is not good
		if(left < minimumLeft){
			//cout<<"On left: "<<left<<" is not enough"<<endl;
			continue;
		}

		int currentRight=center+1;
	
		while(currentRight<(int)y->size()){
			if(y->at(currentRight) >= y->at(currentRight-1)){
				break; // not decreasing
			}

			right++;
			currentRight++;
		}

		// not enough data on the right
		if(right < minimumRight){
			//cout<<"On right: "<<right<<" is not enough"<<endl;
			continue;
		}

		// we found a peak
		int peak=x->at(center);

		#ifdef VERBOSE
		cout<<"findObviousPeak says: "<<peak<<endl;
		#endif

		peakAverages->push_back(peak);
		peakStandardDeviation->push_back(0);

		return;
	}
}


/** find multiple peaks in the distribution of inserts for a library */
void ColoredPeakFinder::findPeaks(vector<int>*x,vector<int>*y,vector<int>*peakAverages,vector<int>*peakStandardDeviation){

	findObviousPeak(x,y,peakAverages,peakStandardDeviation);

	// we found something the easy way
	if(peakAverages->size()>0)
		return;

	vector<int> backgroundData;
	
	for(int i=0;i<(int)y->size();i++){
		if(y->at(i) < CONFIG_SOFT_SIGNAL_THRESHOLD){
			backgroundData.push_back(y->at(i));
		}
	}

	int signalAverage=(int)getAverage(&backgroundData);

	#ifdef VERBOSE
	int signalMode=getMode(&backgroundData);
	cout<<"Mode= "<<signalMode<<" signalAverage= "<<signalAverage<<endl;
	#endif

	int signalThreshold=signalAverage;

	int minimumAccumulatedNoiseSignals=3;
	int minimumAccumulatedWorthySignals=8;
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
				cout<<" accumulatedNoiseSignals="<<accumulatedNoiseSignals<<endl;
				cout<<" accumulatedWorthySignals="<<accumulatedWorthySignals<<endl;
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
		
		// we don't care if there are more than 1 peak
		return;
	}
}


