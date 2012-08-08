/*
 	Ray
    Copyright (C) 2011, 2012  Sébastien Boisvert

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

#ifndef _Amos
#define _Amos

#include <application_core/Parameters.h>
#include <structures/StaticVector.h>
#include <memory/RingAllocator.h>
#include <plugin_FusionData/FusionData.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <plugin_Scaffolder/Scaffolder.h>
#include <plugin_SeedExtender/ReadFetcher.h>
#include <communication/VirtualCommunicator.h>
#include <core/ComputeCore.h>

#include <stdio.h>
#include <stdint.h>
#include <vector>
using namespace std;



/**
 * AMOS specification is available : http://sourceforge.net/apps/mediawiki/amos/index.php?title=AMOS
 * \see http://sourceforge.net/apps/mediawiki/amos/index.php?title=Message_Types
 * \author Sébastien Boisvert
 */
class Amos :  public CorePlugin{

	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS;
	MessageTag RAY_MPI_TAG_ASK_READ_LENGTH;
	MessageTag RAY_MPI_TAG_WRITE_AMOS;
	MessageTag RAY_MPI_TAG_WRITE_AMOS_REPLY;

	MasterMode RAY_MASTER_MODE_AMOS;
	MasterMode RAY_MASTER_MODE_SCAFFOLDER;

	SlaveMode RAY_SLAVE_MODE_AMOS;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;


	VirtualCommunicator*m_virtualCommunicator;
	StaticVector*m_inbox;
	WorkerHandle m_workerId;
	ReadFetcher m_readFetcher;
	int*m_master_mode;
	int*m_slave_mode;
	Scaffolder*m_scaffolder;
	FusionData*m_fusionData;
	FILE*m_amosFile;
	Parameters*m_parameters;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	int m_contigId;
	int m_sequence_id;
	ExtensionData*m_ed;
	int m_mode_send_vertices_sequence_id_position;
	vector<WorkerHandle> m_activeWorkers;
public:
	void call_RAY_MASTER_MODE_AMOS();
	void call_RAY_SLAVE_MODE_AMOS();
	void constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*outbox,
		FusionData*fusionData,ExtensionData*extensionData,int*masterMode,int*slaveMode,Scaffolder*scaffolder,
StaticVector*inbox,VirtualCommunicator*virtualCommunicator);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};


#endif

