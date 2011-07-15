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
using namespace std;

void callPeak(vector<int>*x,vector<int>*y,int peak,int step){
	int middle=(x->at(x->size()-1)-x->at(0))/2;
	int first=x->at(peak)-step;
	int last=x->at(peak)+step;

	if(x->at(peak)>middle && first<middle)
		first=middle;

	if(x->at(peak)<middle && last>middle)
		last=middle;

	int i=0;
	uint64_t sum=0;
	int n=0;
	double thresold=0.001;
	int dataPoints=0;

	while(i<x->size()){
		if(x->at(i)>=first && x->at(i) <= last && y->at(i)> y->at(peak)*thresold){
			sum+=x->at(i)*y->at(i);
			n+=y->at(i);
			dataPoints+=1;
		}
		i+=1;
	}

	int average=sum/n;

	int minimumDatapoints=2;
	if(dataPoints<minimumDatapoints)
		return;

	sum=0;
	i=0;
	while(i<x->size()){
		if(x->at(i)>=first && x->at(i) <= last && y->at(i)> y->at(peak)*thresold){
			int diff=(x->at(i)-average);
			sum+=diff*diff*y->at(i);
		}
		i+=1;
	}

	int standardDeviation=pow(((sum+0.0)/n),0.5);

	first=average-standardDeviation;
	last=average+standardDeviation;
	i=0;
	int busy=0;
	while(i<y->size()){
		if(x->at(i)>=first && x->at(i) <= last)
			busy+=1;
		i+=1;
	}

	double occupancy=(busy+0.0)/(last-first+1)*100;

	int threshold=20;
	if(occupancy<threshold)
		return;

	cout<<"Peak Average= "<<average<<" StandardDeviation= "<<standardDeviation<<" Count= "<<n<<" Points= "<<dataPoints<<" Quality= "<<occupancy<<" %"<<endl;
}

/** find multiple peaks in the distribution of inserts for a library */
void LibraryPeakFinder::findPeaks(vector<int>*x,vector<int>*y,vector<int>*peakAverages,vector<int>*peakStandardDeviation){

	int minI=0;
	int maxI=x->size()-1;

	int minimumCount=5;

	while(minI<x->size() && y->at(minI)<minimumCount)
		minI+=1;

	while(maxI>=0 && y->at(maxI)<minimumCount)
		maxI-=1;

	int minX=x->at(minI);
	int maxX=x->at(maxI);
	int middle=(maxX-minX)/2;

	int i=0;
	int peakLeft=i;
	while(i<x->size() && x->at(i)<middle){
		if(y->at(i)>y->at(peakLeft))
			peakLeft=i;
		i+=1;
	}

	int peakRight=i;
	while(i<x->size() && x->at(i)<=maxX){
		if(y->at(i)>y->at(peakRight))
			peakRight=i;
		i+=1;
	}


	int step=1000;
	if(x->at(peakRight)<x->at(peakLeft)+step){
		int peak=peakLeft;
		if(y->at(peakRight)>y->at(peakLeft))
			peak=peakRight;
		callPeak(x,y,peak,step);
	}else{
		callPeak(x,y,peakLeft,step);
		callPeak(x,y,peakRight,step);
	}
}
