/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

#include <plugin_GeneOntology/KeyEncoder.h>
#include <string.h>

#include <iostream>
using namespace std;


#ifdef ASSERT
#include <assert.h>
#endif

// #define DEBUG_ENCODER

uint64_t KeyEncoder::getPhysicalKmerColor_EMBL_CDS(const char*identifier){

	populateMap();
	

	uint64_t value=encode_EMBL_CDS(identifier);

	#ifdef DEBUG_ENCODER
	cout<<"[encoder] Input: "<<identifier<<" Output: "<<value<<endl;
	#endif

	return value;
}

/*

encoding identifiers for EMBL_CDS

    an identifier is 3 letters and 5 numbers, such as EMBL_CDS:CAA00003 

    the color namespace mask is 10000000000000000 

therefore, the 5 last number can be stored directly as the 5 last numbers

    each of the 3 letter can hold 26 values 

so basically, convert the 3-letter code to base 10, multiply it by 100000 and add the 5 last numbers.

for the 3-letter code, each letter is valued from 0 to 25. 26^3 = 17576

namespace multiplier           10000000000000000
3-letter code multiplier                  100000

the 3-letter code goes from 0 to 17575 and the 5-digit code goes from 0 to 99999

EMBL-CDS can store at most 1 757 582 424 proteins. (uint32_t)

[edit] encoding identifiers for GO

GO:0000001 to are valued from 0 to 9999999 (use uint32_t) 
*/
uint64_t KeyEncoder::encode_EMBL_CDS(const char*identifier){
	/* >EMBL_CDS:CBW26015 CBW26015.1 */

	uint64_t returnValue=0;

	#ifdef ASSERT
	assert(strlen(identifier)==3+5);
	#endif

	uint64_t rightPartValue=0;

	for(int exponent=0;exponent<5;exponent++){
		char symbol=identifier[7-exponent];
		
		#ifdef ASSERT
		assert(m_mapping.count(symbol)>0);
		#endif

		uint64_t value=m_mapping[symbol];

		int loopIterations=exponent;

		while(loopIterations--){
			value*=10;
		}
		
		rightPartValue+=value;
	}

	returnValue+=rightPartValue;

	uint64_t leftPartValue=0;

	for(int exponent=0;exponent<3;exponent++){
		char symbol=identifier[2-exponent];
		
		#ifdef ASSERT
		assert(m_mapping.count(symbol)>0);
		#endif

		uint64_t value=m_mapping[symbol];

		int loopIterations=exponent;

		while(loopIterations--){
			value*=26;
		}
		
		leftPartValue+=value;
	}

	leftPartValue*=100000;

	returnValue+=leftPartValue;

	return returnValue;

}

void KeyEncoder::populateMap(){

	if(m_mapping.size()!=0){
		return;
	}

	int value=0;
	m_mapping['0']=value++;
	m_mapping['1']=value++;
	m_mapping['2']=value++;
	m_mapping['3']=value++;
	m_mapping['4']=value++;
	m_mapping['5']=value++;
	m_mapping['6']=value++;
	m_mapping['7']=value++;
	m_mapping['8']=value++;
	m_mapping['9']=value++;

	value=0;
	m_mapping['A']=value++;
	m_mapping['B']=value++;
	m_mapping['C']=value++;
	m_mapping['D']=value++;
	m_mapping['E']=value++;
	m_mapping['F']=value++;
	m_mapping['G']=value++;
	m_mapping['H']=value++;
	m_mapping['I']=value++;
	m_mapping['J']=value++;
	m_mapping['K']=value++;
	m_mapping['L']=value++;
	m_mapping['M']=value++;
	m_mapping['N']=value++;
	m_mapping['O']=value++;
	m_mapping['P']=value++;
	m_mapping['Q']=value++;
	m_mapping['R']=value++;
	m_mapping['S']=value++;
	m_mapping['T']=value++;
	m_mapping['U']=value++;
	m_mapping['V']=value++;
	m_mapping['W']=value++;
	m_mapping['X']=value++;
	m_mapping['Y']=value++;
	m_mapping['Z']=value++;

}

