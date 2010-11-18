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
#include<OutboxAllocator.h>
#include<PendingRequest.h>
using namespace std;


class MessagesHandler{
	PendingRequest*m_root;
	OutboxAllocator m_customAllocator;
	void printRequests();
public:

	void sendMessages(vector<Message>*outbox,OutboxAllocator*outboxAllocator,vector<Message>*inbox,int rank,MyAllocator*inboxAllocator);
	void receiveMessages(vector<Message>*inbox,MyAllocator*inboxAllocator);
	void addRequest(MPI_Request*request,void*buffer);
	void freeRequests(OutboxAllocator*outboxAllocator);
	MessagesHandler();
};

#endif
