/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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


#include<CoverageDistribution.h>
#include<math.h>
#include<iostream>
#include<fstream>
#include<map>
using namespace std;

double dotProduct(double a,double b,double c,double d){
	return a*c+b*d;
}

double __distance(double qx,double qy,double px,double py,double rx,double ry){
	double pqx=qx-px;
	double pqy=qy-py;
	double prx=rx-px;
	double pry=ry-py;
	double pqpq=dotProduct(pqx,pqy,pqx,pqy);
	double pqpr=dotProduct(pqx,pqy,prx,pry);
	double prpr=dotProduct(prx,pry,prx,pry);
	return sqrt(pqpq-pqpr*pqpr/prpr);
}

double score(int*x,double*y,int i,int j,int c){
	int k=i+1;
	if(k==c)
		return 0;
	double min=y[i];
	if(y[j]<min)
		min=y[j];
	while(k<j){
		if(y[k]<min)
			return 0;
		k++;
	}
	double whole=0;
	k=i;
	while(k<j){
		whole+=(y[k+1]+y[k])*(x[k+1]-x[k]);
		k++;
	}
	double toRemove=(y[i]+y[j])*(x[j]-x[i]);
	double s=whole-toRemove;
	return s;
}

void smooth(double*y,double*smoothy,int c){
	int i=0;
	while(i<c){
		double yval=y[i]*2;
		double n=2.0;
		if(i<c-1){
			yval+=y[i+1];
			n++;
		}else if(i>0){
			yval+=y[i-1];
			n++;
		}
		yval/=n;
		smoothy[i]=yval;
		i++;
	}
}

int findChaos(double*smoothy,int c){
	int i=0;
	double threshold=0.05;
	while(i<c-30){
		int lower=0;
		int higher=0;
		int j=0;
		while(j<30-2){
			double ij=smoothy[i+j];
			double ij1=smoothy[i+j+1];
			double ij2=smoothy[i+j+2];
			if(ij+threshold<ij1 and ij1>ij2+threshold)
				lower++;
			if(ij>ij1+threshold and ij1+threshold<ij2)
				higher++;
			j++;
		}
		if(lower>3 and higher>3){
			return i;
		}
		i++;
	}
	return c-1;
}

void CoverageDistribution::writeFile(map<int,VERTEX_TYPE>*distributionOfCoverage,string*file){
	ofstream f(file->c_str());
	f<<"#Coverage NumberOfVertices"<<endl;
	for(map<int,VERTEX_TYPE>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		#ifdef WRITE_COVERAGE_DISTRIBUTION
		f<<""<<i->first<<" "<<i->second<<endl;
		#endif
	}
	cout<<"\rWriting "<<*file<<""<<endl;
	f.close();
}

CoverageDistribution::CoverageDistribution(map<int,VERTEX_TYPE>*distributionOfCoverage,string*file){
	#ifdef WRITE_COVERAGE_DISTRIBUTION
	writeFile(distributionOfCoverage,file);
	#endif
	int x[255];
	double y[255];
	int c=distributionOfCoverage->size();
	int j=0;
	for(map<int,VERTEX_TYPE>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		x[j]=i->first;
		y[j]=log(i->second);
		j++;
	}
	double smoothy[255];
	smooth(y,smoothy,c);
	int chaos=findChaos(smoothy,c);
	int bestI=0;
	int bestJ=0;
	int bestScore=0;
	for(int i=0;i<chaos;i++){
		for(int j=i+1;j<chaos;j++){
			double s=score(x,y,i,j,c);
			if(s>bestScore){
				bestI=i;
				bestJ=j;
				bestScore=s;
			}
		}
	}
	double max=y[bestI];
	int maxI=bestI;
	for(int i=bestI;i<=bestJ;i++){
		if(y[i]>max){
			maxI=i;
			max=y[i];
		}
	}
	if(maxI==bestI){
		// use vector dot products
		double bestDistance=0;
		for(int i=bestI;i<=bestJ;i++){
			double d=__distance(x[i],y[i],x[bestI],y[bestI],x[bestJ],y[bestJ]);
			if(d>bestDistance){
				maxI=i;
				bestDistance=d;
			}
		}
	}
	m_minimumCoverage=x[bestI];
	m_peakCoverage=x[maxI];
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

