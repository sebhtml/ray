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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _MessagesHandler
#define _MessagesHandler

#include<vector>
#include<MyAllocator.h>
#include<Message.h>
#include<RingAllocator.h>
#include<StaticVector.h>
#include<PendingRequest.h>
using namespace std;


class MessagesHandler{
	int m_messagesSent;
	int m_messagesReceived;
public:
	MessagesHandler();
	void showStats(int rank);
	void sendMessages(StaticVector*outbox,int source);
	void receiveMessages(StaticVector*inbox,RingAllocator*inboxAllocator,int destination);
};

#endif
