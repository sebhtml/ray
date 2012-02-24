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

#include <plugin_PhylogenyViewer/PhylogeneticTreeLoader.h>

#include <assert.h>
#include <iostream>
using namespace std;

void PhylogeneticTreeLoader::load(string file){

	STEPPING=1000000;

	m_current=0;
	m_size=0;

	m_stream.open(file.c_str());
	
	if(!m_stream){
		cout<<"File "<<file<<" is invalid."<<endl;

		m_stream.close();

	}
	
	int count=0;

	while(!m_stream.eof()){
		string a="";

		m_stream>>a;

		if(a!=""){
			count++;

			if(count==2){
				count=0;
				m_size++;

				if(m_size % STEPPING == 0){
					cout<<"PhylogeneticTreeLoader::load "<<m_size<<endl;
				}
			}
		}

	}

	m_stream.close();

	m_stream.open(file.c_str());

	cout<<"File "<<file<<" has "<<m_size<<" entries"<<endl;
}

bool PhylogeneticTreeLoader::hasNext(){
	return m_current<m_size;
}

void PhylogeneticTreeLoader::getNext(uint64_t*parent,uint64_t*child){

	if(m_current % STEPPING == 0){
		cout<<"PhylogeneticTreeLoader::getNext "<<m_current<<"/"<<m_size<<endl;
	}

	uint64_t l1;
	uint64_t l2;

	m_stream>>l1>>l2;

	*parent=l1;
	*child=l2;

	m_current++;
}

