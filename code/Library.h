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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#ifndef _Library
#define _Library

#include<BufferedData.h>
#include<ExtensionData.h>
#include<common_functions.h>
#include<map>
#include<StaticVector.h>
#include<TimePrinter.h>
#include<Parameters.h>
#include<SeedingData.h>
#include<RingAllocator.h>
using namespace std;

class Library{
	BufferedData*m_bufferedData;
	bool m_ready;
	int m_rank;
	StaticVector*m_outbox;
	int*m_sequence_id;
	int*m_sequence_idInFile;
	ExtensionData*m_ed;
	map<u64,int>*m_readsPositions;
	map<u64,char> m_readsStrands;
	RingAllocator*m_outboxAllocator;
	int m_size;
	TimePrinter*m_timePrinter;
	int*m_mode;
	int*m_master_mode;
	Parameters*m_parameters;
	int*m_fileId;
	SeedingData*m_seedingData;
	map<int,map<int,int> >*m_libraryDistances;

public:
	void updateDistances();
	void setReadiness();
	int getRank();
	int getSize();
	void detectDistances();
	void constructor(int m_rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator,BufferedData*m_bufferedData,
	int*m_sequence_id,int*m_sequence_idInFile,ExtensionData*m_ed,map<u64,int >*m_readsPositions,int m_size,
TimePrinter*m_timePrinter,int*m_mode,int*m_master_mode,
Parameters*m_parameters,int*m_fileId,SeedingData*m_seedingData,map<int,map<int,int> >*m_libraryDistances

);
};

#endif
