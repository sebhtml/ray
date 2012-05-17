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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_SeedExtender/Direction.h>
#include <stdlib.h>

void Direction::constructor(PathHandle wave,int progression,bool lower){
	m_lower=lower;
	m_wave=wave;
	m_progression=progression;
	m_next=NULL;
}

PathHandle Direction::getWave(){
	return m_wave;
}

int Direction::getProgression(){
	return m_progression;
}

Direction*Direction::getNext(){
	return m_next;
}

void Direction::setNext(Direction*e){
	#ifdef ASSERT
	assert(this!=NULL);

	if(e!=NULL){
		Direction copy=*e;
		assert(copy.getNext()==NULL || copy.getNext()!=NULL);
	}
	#endif

	m_next=e;
}

bool Direction::isLower(){
	return m_lower;
}
