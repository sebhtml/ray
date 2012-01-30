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

#include <plugin_SequencesLoader/Read.h>
#include <plugin_SeedExtender/ExtensionElement.h>
#include <string.h>

/** set the sequence */
void ExtensionElement::setSequence(const char*b,MyAllocator*allocator){
	m_read.constructor(b,allocator,false);
	m_hasPairedRead=false;
}

void ExtensionElement::getSequence(char*sequence,Parameters*parameters){
	m_read.getSeq(sequence,parameters->getColorSpaceMode(),false);
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
	/* TODO:  not implemented */
}

void ExtensionElement::constructor(){
	m_canMove=true;

	m_agreement=0;
}

bool ExtensionElement::canMove(){
	return m_canMove;
}

void ExtensionElement::freezePlacement(){
	m_canMove=false;
}

void ExtensionElement::increaseAgreement(){
	m_agreement ++ ;
}

int ExtensionElement::getAgreement(){
	return m_agreement;
}

int ExtensionElement::getReadLength(){
	return m_read.length();
}
