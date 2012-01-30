/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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

#include<iostream>
#include<string.h>
#include<plugin_SequencesLoader/ColorSpaceDecoder.h>
#include<stdlib.h>
using namespace std;

#define _COLOR_SPACE_DECODER_CODE_A 0
#define _COLOR_SPACE_DECODER_CODE_C 1
#define _COLOR_SPACE_DECODER_CODE_G 2
#define _COLOR_SPACE_DECODER_CODE_T 3
#define _COLOR_SPACE_DECODER_COLOR_BLUE '0'
#define _COLOR_SPACE_DECODER_COLOR_GREEN '1'
#define _COLOR_SPACE_DECODER_COLOR_ORANGE '2'
#define _COLOR_SPACE_DECODER_COLOR_RED '3'
#define _COLOR_SPACE_DECODER_LETTER_A 'A'
#define _COLOR_SPACE_DECODER_LETTER_T 'T'
#define _COLOR_SPACE_DECODER_LETTER_C 'C'
#define _COLOR_SPACE_DECODER_LETTER_G 'G'
#define _COLOR_SPACE_DECODER_NUMBER_OF_COLORS 4

/*
 * see http://www.ploscompbiol.org/article/slideshow.action?uri=info:doi/10.1371/journal.pcbi.1000386&imageURI=info:doi/10.1371/journal.pcbi.1000386.g002
 */
ColorSpaceDecoder::ColorSpaceDecoder(){
	int i=0;

	// A-dependant colors
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_BLUE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_GREEN;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_ORANGE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_RED;

	// C-dependant colors
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_GREEN;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_BLUE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_RED;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_ORANGE;

	// G-dependant colors
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_ORANGE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_RED;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_BLUE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_GREEN;

	// T-dependant colors
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_RED;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_ORANGE;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_GREEN;
	m_colors[i++]=_COLOR_SPACE_DECODER_COLOR_BLUE;
}

/*
 * decode color-space read.
 */
string ColorSpaceDecoder::decode(char*x){
	int len=strlen(x);
	char v[1024];
	int lastCode=0;
	char boot=x[0];
	if(boot==_COLOR_SPACE_DECODER_LETTER_A){
		lastCode=_COLOR_SPACE_DECODER_CODE_A;
	}else if(boot==_COLOR_SPACE_DECODER_LETTER_T){
		lastCode=_COLOR_SPACE_DECODER_CODE_T;
	}else if(boot==_COLOR_SPACE_DECODER_LETTER_C){
		lastCode=_COLOR_SPACE_DECODER_CODE_C;
	}else if(boot==_COLOR_SPACE_DECODER_LETTER_G){
		lastCode=_COLOR_SPACE_DECODER_CODE_G;
	}
	int i=1;
	while(i<len){
		char color=x[i];
		int currentCode=0;
		while(currentCode<_COLOR_SPACE_DECODER_NUMBER_OF_COLORS){
			if(color==m_colors[lastCode*_COLOR_SPACE_DECODER_NUMBER_OF_COLORS+currentCode]){
				break;
			}
			currentCode++;
		}
		if(currentCode==_COLOR_SPACE_DECODER_CODE_A){
			v[i-1]=_COLOR_SPACE_DECODER_LETTER_A;
		}else if(currentCode==_COLOR_SPACE_DECODER_CODE_C){
			v[i-1]=_COLOR_SPACE_DECODER_LETTER_C;
		}else if(currentCode==_COLOR_SPACE_DECODER_CODE_G){
			v[i-1]=_COLOR_SPACE_DECODER_LETTER_G;
		}else if(currentCode==_COLOR_SPACE_DECODER_CODE_T){
			v[i-1]=_COLOR_SPACE_DECODER_LETTER_T;
		}
		lastCode=currentCode;
		i++;
	}
	v[len]='\0';
	string s(v);
	return s;
}
