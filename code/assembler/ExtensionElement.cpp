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

#include <structures/Read.h>
#include <assembler/ExtensionElement.h>
#include <string.h>

void ExtensionElement::setSequence(const char*b,MyAllocator*allocator){
	m_readSequence=(char*)allocator->allocate((strlen(b)+1)*sizeof(char));
	strcpy(m_readSequence,b);
	m_hasPairedRead=false;
}

char*ExtensionElement::getSequence(){
	return m_readSequence;
}

int ExtensionElement::getPosition(){
	return m_position;
}

char ExtensionElement::getStrand(){
	return m_strand;
}

bool ExtensionElement::hasPairedRead(){
	return m_hasPairedRead;
}

PairedRead*ExtensionElement::getPairedRead(){
	return &m_pairedRead;
}

void ExtensionElement::setStartingPosition(int a){
	m_position=a;
}

void ExtensionElement::setStrand(char a){
	m_strand=a;
}

void ExtensionElement::setType(int a){
	m_type=a;
}

bool ExtensionElement::isLeftEnd(){
	return m_type==TYPE_LEFT_END;
}

bool ExtensionElement::isRightEnd(){
	return m_type==TYPE_RIGHT_END;
}

int ExtensionElement::getType(){
	return m_type;
}

void ExtensionElement::setPairedRead(PairedRead a){
	m_pairedRead=a;
	m_hasPairedRead=true;
}

void ExtensionElement::setStrandPosition(int a){
	m_strandPosition=a;
}

int ExtensionElement::getStrandPosition(){
	return m_strandPosition;
}

void ExtensionElement::removeSequence(){
	m_readSequence=NULL;
}
