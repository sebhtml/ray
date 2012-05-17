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

#include <plugin_Scaffolder/SummarizedLink.h>

SummarizedLink::SummarizedLink(PathHandle leftContig,Strand leftStrand,PathHandle rightContig,
	Strand rightStrand,int average,int count,
	int standardDeviation){

	m_standardDeviation=standardDeviation;
	m_leftContig=leftContig;
	m_leftStrand=leftStrand;
	m_rightContig=rightContig;
	m_rightStrand=rightStrand;
	m_average=average;
	m_count=count;
}

PathHandle SummarizedLink::getLeftContig(){
	return m_leftContig;
}

Strand SummarizedLink::getLeftStrand(){
	return m_leftStrand;
}

PathHandle SummarizedLink::getRightContig(){
	return m_rightContig;
}

Strand SummarizedLink::getRightStrand(){
	return m_rightStrand;
}

int SummarizedLink::getAverage(){
	return m_average;
}

int SummarizedLink::getCount(){
	return m_count;
}

void SummarizedLink::pack(MessageUnit*buffer,int*position){
	buffer[(*position)++]=getLeftContig();
	buffer[(*position)++]=getLeftStrand();
	buffer[(*position)++]=getRightContig();
	buffer[(*position)++]=getRightStrand();
	buffer[(*position)++]=getCount();
	buffer[(*position)++]=getAverage();
	buffer[(*position)++]=getStandardDeviation();
}

void SummarizedLink::unpack(MessageUnit*buffer,int*position){
	m_leftContig=buffer[(*position)++];
	m_leftStrand=buffer[(*position)++];
	m_rightContig=buffer[(*position)++];
	m_rightStrand=buffer[(*position)++];
	m_count=buffer[(*position)++];
	m_average=buffer[(*position)++];
	m_standardDeviation=buffer[(*position)++];
}

SummarizedLink::SummarizedLink(){
}

int SummarizedLink::getStandardDeviation(){
	return m_standardDeviation;
}
