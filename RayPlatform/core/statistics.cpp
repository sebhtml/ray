/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <core/statistics.h>
#include <math.h> /* for sqrt */
#include <stdint.h>
#include <map>
using namespace std;

double getAverage(vector<int>*values){
	int i=0;
	int n=values->size();
	int sum=0;
	while(i<n){
		sum+=values->at(i);
		i++;
	}

	double average=sum;

	if(n > 0){
		average /= n;
	}

	return average;
}

/** compute the standard deviation */
double getStandardDeviation(vector<int>*x){
	int i=0;

	double averageValue=getAverage(x);
	uint64_t sum=0;

	int n=x->size();

	while(i<n){
		int diff=(int)(x->at(i)-averageValue);
		sum+=diff*diff;
		i++;
	}

	double standardDeviation=sqrt(sum+0.0);
	
	if(n > 0){
		standardDeviation=sqrt((sum+0.0)/n);
	}

	return standardDeviation;
}

int getMode(vector<int>*x){
	map<int,int> data;
	for(int i=0;i<(int)x->size();i++){
		data[x->at(i)]++;
	}
	int best=-1;

	for(map<int,int>::iterator i=data.begin();
		i!=data.end();i++){
		if(data.count(best)==0 || i->second > data[best]){
			best=i->first;
		}
	}

	return best;
}
