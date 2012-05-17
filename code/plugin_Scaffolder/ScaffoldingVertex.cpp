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

#include <plugin_Scaffolder/ScaffoldingVertex.h>
#include <sstream>
#include <string>
#include <iostream>
using namespace std;

ScaffoldingVertex::ScaffoldingVertex(PathHandle name,int length){
	m_name=name;
	m_length=length;
}

PathHandle ScaffoldingVertex::getName(){
	return m_name;
}

int ScaffoldingVertex::getLength(){
	return m_length;
}

void ScaffoldingVertex::read(ifstream*f){
	/*
 
contig-408      4698

*/
	string token;
	(*f)>>token;
	string number=token.substr(7,token.length()-7);
	stringstream aStream;
	aStream << number;
	aStream >> m_name;
	(*f) >> m_length;
}

ScaffoldingVertex::ScaffoldingVertex(){
}

