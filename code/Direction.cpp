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

#include<Direction.h>
#include<stdlib.h>

void Direction::constructor(uint64_t wave,int progression){
	m_wave=wave;
	m_progression=progression;
	m_next=NULL;
}

uint64_t Direction::getWave(){
	return m_wave;
}

int Direction::getProgression(){
	return m_progression;
}

Direction*Direction::getNext(){
	return m_next;
}

void Direction::setNext(Direction*e){
	m_next=e;
}

