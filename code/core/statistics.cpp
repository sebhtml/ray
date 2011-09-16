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

#include <core/statistics.h>
#include <math.h> /* for sqrt */
#include <stdint.h>

int getAverage(vector<int>*values){
	int i=0;
	int n=values->size();
	int sum=0;
	while(i<n){
		sum+=values->at(i);
		i++;
	}

	int average=sum/n;
	return average;
}

/** compute the standard deviation */
int getStandardDeviation(vector<int>*x){
	int i=0;

	int averageValue=getAverage(x);
	uint64_t sum=0;

	int n=x->size();

	while(i<n){
		int diff=(x->at(i)-averageValue);
		sum+=diff*diff;
		i++;
	}

	int standardDeviation=(int)sqrt((sum+0.0)/n);
	
	return standardDeviation;
}
