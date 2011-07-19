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

#include <pairs/LibraryPeakFinder.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
using namespace std;

/* find the peak */
void callPeak(vector<int>*x,vector<int>*y,int peak,vector<int>*peakAverages,vector<int>*peakStandardDeviation,
	int minX,int maxX){
	int step=5000;
	int middle=(maxX-minX)/2;
	int first=x->at(peak)-step;
	int last=x->at(peak)+step;

	/** correct the first */
	if(x->at(peak)>middle && first<middle)
		first=middle;

	/** correct the last */
	if(x->at(peak)<middle && last>middle)
		last=middle;

	int i=0;
	uint64_t sum=0;
	int n=0;
	double thresold=0.001;
	int dataPoints=0;

	//cout<<"first "<<first<<" last "<<last<<endl;
	/** compute the average around the possible peak */
	while(i<(int)x->size()){
		if(x->at(i)>=first && x->at(i) <= last && y->at(i)> y->at(peak)*thresold){
			sum+=x->at(i)*y->at(i);
			n+=y->at(i);
			dataPoints+=1;
		}
		i+=1;
	}

	int average=sum/n;

	int minimumDatapoints=2;

	//cout<<"points "<<dataPoints<<endl;
	/** requires at least 2 data points */
	if(dataPoints<minimumDatapoints)
		return;

	sum=0;
	i=0;

	/** compute the standard deviation */
	while(i<(int)x->size()){
		if(x->at(i)>=first && x->at(i) <= last && y->at(i)> y->at(peak)*thresold){
			int diff=(x->at(i)-average);
			sum+=diff*diff*y->at(i);
		}
		i+=1;
	}

	int standardDeviation=(int)pow(((sum+0.0)/n),0.5);

	/** compute the quality of the peak */
	first=average-standardDeviation;
	last=average+standardDeviation;
	i=0;
	int busy=0;
	while(i<(int)y->size()){
		if(x->at(i)>=first && x->at(i) <= last)
			busy+=1;
		i+=1;
	}

	double occupancy=(busy+0.0)/(last-first+1)*100;

	/* minimum quality is thresold % */
	int threshold=20;
	if(occupancy<threshold)
		return;

	cout<<"Peak Average= "<<average<<" StandardDeviation= "<<standardDeviation<<" Count= "<<n<<" Points= "<<dataPoints<<" Quality= "<<occupancy<<" %"<<endl;

	peakAverages->push_back(average);
	peakStandardDeviation->push_back(standardDeviation);
}

/** find multiple peaks in the distribution of inserts for a library */
void LibraryPeakFinder::findPeaks(vector<int>*x,vector<int>*y,vector<int>*peakAverages,vector<int>*peakStandardDeviation){
	/** if this is simulated data with no standard deviation.*/
	int lowQuality=0;
	int highQuality=0;
	int lowThreshold=500;
	int highThreshold=10000;
	int qualityPeak=0;
	for(int i=0;i<(int)y->size();i++){
		if(y->at(i)<lowThreshold){
			lowQuality++;
		}else if(y->at(i)>highThreshold){
			highQuality++;
			qualityPeak=x->at(i);
		}
	}
	cout<<"Low= "<<lowQuality<<" High= "<<highQuality<<endl;
	if(((lowQuality+highQuality) == (int)y->size()) && (highQuality == 1)){
		cout<<"No deviation! Peak 0 "<<qualityPeak<<" 0"<<endl;
		/** only one data point was interesting actually */
		peakAverages->push_back(qualityPeak);
		peakStandardDeviation->push_back(0);
		return;
	}

	/** data is more complex, needs some serious data analysis */

	int minI=0;
	int maxI=x->size()-1;

	int minimumCount=5;

	while(minI<(int)x->size() && y->at(minI)<minimumCount)
		minI+=1;

	while(maxI>=0 && y->at(maxI)<minimumCount)
		maxI-=1;

	int minX=x->at(minI);
	int maxX=x->at(maxI);
	int middle=(maxX-minX)/2;

	int i=0;
	int peakLeft=i;
	/** find a peak on the left side */
	while(i<(int)x->size() && x->at(i)<middle){
		if(y->at(i)>y->at(peakLeft))
			peakLeft=i;
		i+=1;
	}

	int peakRight=i;
	/** find a peak on the right side */
	while(i<(int)x->size() && x->at(i)<=maxX){
		if(y->at(i)>y->at(peakRight))
			peakRight=i;
		i+=1;
	}

	/** peaks must be at least 1000 apart */
	int step=1000;

	/* there is one peak */
	if(x->at(peakRight)<x->at(peakLeft)+step){
		//cout<<"1 peak"<<endl;
		int peak=peakLeft;
		if(y->at(peakRight)>y->at(peakLeft))
			peak=peakRight;
		callPeak(x,y,peak,peakAverages,peakStandardDeviation,minX,maxX);

	/* there are two peaks */
	}else{
		//cout<<"2 peaks "<<x->at(peakLeft)<<" and "<<x->at(peakRight)<<endl;
		callPeak(x,y,peakLeft,peakAverages,peakStandardDeviation,minX,maxX);
		callPeak(x,y,peakRight,peakAverages,peakStandardDeviation,minX,maxX);
	}
}
