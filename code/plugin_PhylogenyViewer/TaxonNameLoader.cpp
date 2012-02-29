/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <plugin_PhylogenyViewer/TaxonNameLoader.h>
#include <plugin_PhylogenyViewer/types.h>

#include <assert.h>
#include <string.h>
#include <sstream>
#include <iostream>
using namespace std;

void TaxonNameLoader::load(string file){

	m_current=0;
	m_size=0;

	m_stream.open(file.c_str());
	
	if(!m_stream){
		cout<<"File "<<file<<" is invalid."<<endl;

		m_stream.close();

	}
	
	while(!m_stream.eof()){
		char line[1024];
		line[0]='\0';
		m_stream.getline(line,1024);

		if(strlen(line)>0){
			m_size++;
		}
	}

	m_stream.close();

	m_stream.open(file.c_str());

	cout<<"File "<<file<<" has "<<m_size<<" entries"<<endl;
}

bool TaxonNameLoader::hasNext(){
	return m_current<m_size;
}

void TaxonNameLoader::getNext(TaxonIdentifier*taxon,string*name,string*rank){

	char line[1024];
	line[0]='\0';
	m_stream.getline(line,1024);

	string theLine=line;

	int firstTab=0;
	int secondTab=0;

	int tabSymbols=0;

	while(firstTab<(int)theLine.length() && tabSymbols!=1){
		if(theLine[firstTab]=='\t'){
			tabSymbols++;
		}
		firstTab++;
	}

	tabSymbols=0;

	while(secondTab<(int)theLine.length() && tabSymbols!=2){
		if(theLine[secondTab]=='\t'){
			tabSymbols++;
		}
		secondTab++;
	}


	TaxonIdentifier theTaxon;
	string theName;

	istringstream virtualStream;
	virtualStream.str(theLine);
	virtualStream>>theTaxon;

	int symbols=secondTab-firstTab-1;
	theName=theLine.substr(firstTab,symbols);

	(*taxon)=theTaxon;
	(*name)=theName;

	(*rank)=theLine.substr(secondTab);

	m_current++;

	if(m_current==m_size){
		m_stream.close();
	}
}

