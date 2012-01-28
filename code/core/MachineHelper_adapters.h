/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#ifndef _MachineHelper_adapters_h
#define _MachineHelper_adapters_h

#include <handlers/MasterModeHandler.h>
#include <handlers/SlaveModeHandler.h>

class MachineHelper;

class Adapter_RAY_MASTER_MODE_LOAD_CONFIG: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_WRITE_KMERS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_LOAD_SEQUENCES: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_INDEXING: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_PREPARE_SEEDING: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_SEEDING: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_DETECTION: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_ASK_DISTANCES: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_START_FUSION_CYCLE: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_ASK_EXTENSIONS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_SCAFFOLDER: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_KILL_RANKS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};


class Adapter_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_EXTRACT_VERTICES: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_PURGE_NULL_EDGES: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_WRITE_KMERS: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_DIE: public SlaveModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS: public MasterModeHandler{
	MachineHelper*m_object;

public:
	void setObject(MachineHelper*object);
	void call();
};




#endif
