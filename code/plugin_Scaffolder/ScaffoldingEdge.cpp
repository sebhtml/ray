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

#include <plugin_Scaffolder/ScaffoldingEdge.h>
#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>
using namespace std;

ScaffoldingEdge::ScaffoldingEdge(PathHandle leftContig,Strand leftStrand,PathHandle rightContig,Strand rightStrand,int gapSize,
	int average1,int count1,int standardDeviation1,
	int average2,int count2,int standardDeviation2){
	m_gapSize=gapSize;
	m_leftContig=leftContig;
	m_leftStrand=leftStrand;
	m_rightContig=rightContig;
	m_rightStrand=rightStrand;

	m_count1=count1;
	m_average1=average1;
	m_standardDeviation1=standardDeviation1;

	m_count2=count2;
	m_average2=average2;
	m_standardDeviation2=standardDeviation2;
}

ScaffoldingEdge ScaffoldingEdge::getReverseEdge(){

	char reverseLeft='F';
	if(m_leftStrand=='F')
		reverseLeft='R';

	char reverseRight='F';
	if(m_rightStrand=='F')
		reverseRight='R';

	ScaffoldingEdge e2(m_rightContig,reverseRight,m_leftContig,reverseLeft,m_gapSize,m_average2,m_count2,m_standardDeviation2,m_average1,m_count1,m_standardDeviation1);
	
	return e2;
}

PathHandle ScaffoldingEdge::getLeftContig(){
	return m_leftContig;
}

Strand ScaffoldingEdge::getLeftStrand(){
	return m_leftStrand;
}

PathHandle ScaffoldingEdge::getRightContig(){
	return m_rightContig;
}

Strand ScaffoldingEdge::getRightStrand(){
	return m_rightStrand;
}

int ScaffoldingEdge::getAverage1(){
	return m_average1;
}

int ScaffoldingEdge::getCount1(){
	return m_count1;
}

int ScaffoldingEdge::getStandardDeviation1(){
	return m_standardDeviation1;
}

int ScaffoldingEdge::getAverage2(){
	return m_average2;
}

int ScaffoldingEdge::getCount2(){
	return m_count2;
}

int ScaffoldingEdge::getStandardDeviation2(){
	return m_standardDeviation2;
}

void ScaffoldingEdge::read(ifstream*f){
	/*
version 1

contig-0        F       contig-2000306  F       7414    2       0       7384    40      1       7444    37

version 2

contig-0        R       contig-356000158        F       9012    2       0       3       9012    0       1       3       9012    0


*/
	{
	string token;
	(*f)>>token;
	string number=token.substr(7,token.length()-7);
	stringstream aStream;
	aStream << number;
	aStream >> m_leftContig;
	}

	(*f) >> m_leftStrand;

	{
	string token;
	(*f)>>token;
	string number=token.substr(7,token.length()-7);
	stringstream aStream;
	aStream << number;
	aStream >> m_rightContig;
	}

	(*f) >> m_rightStrand;
	(*f) >> m_gapSize;

	int dummy;
	(*f) >> dummy; /* count */
	(*f) >> dummy; /* index */

	(*f) >> m_count1;
	(*f) >> m_average1;
	(*f) >> m_standardDeviation1;

	(*f) >> dummy; /* index */

	(*f) >> m_count2;
	(*f) >> m_average2;
	(*f) >> m_standardDeviation2;
}

ScaffoldingEdge::ScaffoldingEdge(){
}

int ScaffoldingEdge::getPriority()const {
	return (m_count1+m_count2)/2;
}

bool ScaffoldingEdge::operator<(const ScaffoldingEdge&other)const{
	return getPriority()<other.getPriority();
}

void ScaffoldingEdge::print(){
	cout<<m_leftContig<<" "<<m_leftStrand<<" "<<m_rightContig<<" "<<m_rightStrand<<" gap= "<<m_gapSize<<" priority= "<<getPriority()<<endl;
}

Strand ScaffoldingEdge::getStrand(PathHandle name){
	#ifdef ASSERT
	assert(name==getLeftContig() || name==getRightContig());
	#endif

	if(name==getLeftContig())
		return getLeftStrand();
	else if(name==getRightContig())
		return getRightStrand();

	return 'x';
}

int ScaffoldingEdge::getSide(PathHandle name){
	#ifdef ASSERT
	assert(name==getLeftContig() || name==getRightContig());
	#endif

	if(name==getLeftContig())
		return  LEFT_SIDE; 
	else if(name==getRightContig())
		return RIGHT_SIDE;

	return -1;
}

int ScaffoldingEdge::getGapSize(){
	return m_gapSize;
}
