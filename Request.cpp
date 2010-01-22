/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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


#include"Request.h"
#include<iostream>
using namespace std;

Request::Request(MessageToSend m,MPI_Request request){
	m_numberOfTests=0;
	m_message=m;
	m_request=request;
}

void Request::addTest(){
	m_numberOfTests++;
}

MPI_Request Request::getMPIRequest(){
	return m_request;
}

int Request::getNumberOfTests(){
	return m_numberOfTests;
}

void Request::print(){
	cout<<"Destination="<<m_message.getDestination()<<endl;
}
