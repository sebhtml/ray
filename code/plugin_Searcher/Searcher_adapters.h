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

#ifndef _Searcher_adapters_h
#define _Searcher_adapters_h

#include <handlers/SlaveModeHandler.h>
#include <handlers/MasterModeHandler.h>
#include <handlers/MessageTagHandler.h>

class Searcher;

class Adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS: public SlaveModeHandler{
	Searcher*m_object;
public:
	void setObject(Searcher*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES: public SlaveModeHandler{
	Searcher*m_object;
public:
	void setObject(Searcher*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES: public SlaveModeHandler{
	Searcher*m_object;
public:
	void setObject(Searcher*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_ADD_COLORS: public SlaveModeHandler{
	Searcher*m_object;
public:
	void setObject(Searcher*object);
	void call();
};


class Adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS: public MasterModeHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call();
};

class Adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES: public MasterModeHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call();
};

class Adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES: public MasterModeHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call();
};

class Adapter_RAY_MASTER_MODE_ADD_COLORS: public MasterModeHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call();
};

class Adapter_RAY_MPI_TAG_ADD_KMER_COLOR: public MessageTagHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS: public MessageTagHandler{
        Searcher*m_object;
public:
        void setObject(Searcher*object);
        void call(Message*message);
};

#endif

