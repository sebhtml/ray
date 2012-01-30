/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include <plugin_Scaffolder/ScaffoldingLink.h>

ScaffoldingLink::ScaffoldingLink(){
}

void ScaffoldingLink::constructor(int distance,int coverage1,int coverage2){
	m_distance=distance;
	m_path1MarkerCoverage=coverage1;
	m_path2MarkerCoverage=coverage2;
}

int ScaffoldingLink::getDistance(){
	return m_distance;
}

int ScaffoldingLink::getCoverage1(){
	return m_path1MarkerCoverage;
}

int ScaffoldingLink::getCoverage2(){
	return m_path2MarkerCoverage;
}

