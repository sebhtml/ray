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


#ifndef _Request
#define _Request

#include<mpi.h>
#include"Message.h"

class Request{
	Message m_message;
	int m_numberOfTests;
	MPI_Request m_request;
public:
	Request(Message m,MPI_Request m_request);
	MPI_Request getMPIRequest();
	void addTest();
	int getNumberOfTests();
	void print();
	Message*getMessage();
};

#endif

