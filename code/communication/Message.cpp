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

#include <communication/Message.h>

Message::Message(void*b,int c,MPI_Datatype d,int dest,int tag,int source){
	// buffer must be allocated or else it will CORE DUMP.
	m_buffer=b;
	m_count=c;
	m_datatype=d;
	m_dest=dest;
	m_tag=tag;
	m_source=source;
}

void*Message::getBuffer(){
	return m_buffer;
}

int Message::getCount(){
	return m_count;
}

MPI_Datatype Message::getMPIDatatype(){
	return m_datatype;
}

int Message::getDestination(){
	return m_dest;
}

int Message::getTag(){
	return m_tag;
}

Message::Message(){
}

int Message::getSource(){
	return m_source;
}

void Message::setBuffer(void*a){
	m_buffer=a;
}
