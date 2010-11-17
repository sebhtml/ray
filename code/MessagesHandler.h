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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _MessagesHandler
#define _MessagesHandler

#include<vector>
#include<MyAllocator.h>
#include<Message.h>
using namespace std;

class Request{
public:
	MPI_Request*m_mpiRequest;
	Request*m_next;
};

class MessagesHandler{
	Request*m_root;
	void printRequests();
public:

	void sendMessages(vector<Message>*outbox,MyAllocator*outboxAllocator);
	void receiveMessages(vector<Message>*inbox,MyAllocator*inboxAllocator);
	void addRequest(MPI_Request*request);
	void freeRequests(MyAllocator*outboxAllocator);
	MessagesHandler();
};

#endif
