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

#include <communication/Message.h>
#include <iostream>
#include <communication/mpi_tags.h>
using namespace std;

/** buffer must be allocated or else it will CORE DUMP. */
Message::Message(uint64_t*b,int c,Rank dest,MessageTag tag,Rank source){
	m_buffer=b;
	m_count=c;
	m_destination=dest;
	m_tag=tag;
	m_source=source;
}

uint64_t*Message::getBuffer(){
	return m_buffer;
}

int Message::getCount(){
	return m_count;
}

Rank Message::getDestination(){
	return m_destination;
}

MessageTag Message::getTag(){
	return m_tag;
}

Message::Message(){}

int Message::getSource(){
	return m_source;
}

void Message::print(){
	uint8_t shortTag=getTag();

	cout<<"Source: "<<getSource()<<" Destination: "<<getDestination()<<" Tag: "<<MESSAGE_TAGS[shortTag]<<" Count: "<<getCount();
	if(m_count > 0){
		cout<<" Overlay: "<<hex<<m_buffer[0]<<dec;
	}
}

void Message::setBuffer(uint64_t*buffer){
	m_buffer = buffer;
}

void Message::setTag(MessageTag tag){
	m_tag=tag;
}

void Message::setSource(Rank source){
	m_source=source;
}

void Message::setDestination(Rank destination){
	m_destination=destination;
}
