/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include"ReadAnnotation.h"
#include<cstdlib>

void ReadAnnotation::constructor(int a,int b){
	m_rank=a;
	m_readIndex=b;
	m_next=NULL;
}

int ReadAnnotation::getRank(){
	return m_rank;
}

int ReadAnnotation::getReadIndex(){
	return m_readIndex;
}

void ReadAnnotation::setNext(ReadAnnotation*a){
	m_next=a;
}

ReadAnnotation*ReadAnnotation::getNext(){
	return m_next;
}

