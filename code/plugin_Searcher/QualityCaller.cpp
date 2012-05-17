/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <plugin_Searcher/QualityCaller.h>
#include <core/statistics.h>

#include <iostream>
#include <assert.h>
#include <math.h>
using namespace std;

/*
 * \see http://en.wikipedia.org/wiki/Pearson_product-moment_correlation_coefficient
 */
double QualityCaller::computeCorrelation(vector<int>*x,vector<int>*y){
	if(x->size()==0)
		return 0;

	#ifdef ASSERT
	assert(x->size()==y->size());
	#endif

	double averageX=getAverage(x);
	double averageY=getAverage(y);

	double firstSum=0;
	double deviationX=0;
	double deviationY=0;

	for(int i=0;i<(int)x->size();i++){
		int xValue=x->at(i);
		int yValue=y->at(i);

		double diffX= xValue - averageX;
		double diffY= yValue - averageY;

		firstSum+= diffX * diffY;

		deviationX+= diffX*diffX;
		deviationY+= diffY*diffY;
	}

	if(deviationX==0 || deviationY==0)
		return 0;

	double correlation=(0.0+firstSum)/(sqrt(0.0+deviationX) * sqrt(0.0+deviationY));

	return correlation;
}

double QualityCaller::computeQuality(map<CoverageDepth,LargeCount>*array1,map<CoverageDepth,LargeCount>*array2){
	vector<int> y1Values;
	vector<int> y2Values;

	#ifdef CONFIG_CALLER_VERBOSE
	cout<<"peakY for array1 "<<bestX<<endl;
	#endif

	for(map<CoverageDepth,LargeCount>::iterator i=array1->begin();i!=array1->end();i++){
		int x1=i->first;
		int y1=i->second;
		

		/* array2 should alwayas have this point */
		if(array2->count(x1) > 0){

			int y2=(*array2)[x1];

			#ifdef CONFIG_CALLER_VERBOSE_POINTS
			cout<<"POINT	"<<x1<<"	"<<y1<<"	"<<y2<<endl;
			#endif

			y1Values.push_back(y1);
			y2Values.push_back(y2);
		}
	}

	double correlation=computeCorrelation(&y1Values,&y2Values);

	#ifdef CONFIG_CALLER_VERBOSE
	cout<<"Correlation computed on "<<y1Values.size()<<" points."<<endl;
	#endif

	if(correlation<0)
		correlation=-correlation;

	/* the quality score is the absolute correlation coefficient */
	return correlation;
}


