/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

\file GenomeNeighbourhood.h
\author Sébastien Boisvert
*/


#ifndef _GenomeNeighbourhood_h
#define _GenomeNeighbourhood_h

#include <profiling/TimePrinter.h>
#include <core/ComputeCore.h>
#include <plugins/CorePlugin.h>
#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood_adapters.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>

#include <vector>
#include <string>
#include <stdint.h> /* for uint64_t */
using namespace std;

/**
 * The plugin GenomeNeighbourhood outputs a file file
 * containing contig neighbourhoods.
 *
 * This is useful to know where is located a drug-resistance gene,
 * amongst other things.
 *
 * \author Sébastien Boisvert
 * \
 * */
class GenomeNeighbourhood: public CorePlugin{

	bool m_started;
	bool m_slaveStarted;
	int m_contigIndex;
	bool m_doneLeftSide;
	bool m_doneRightSide;


	ComputeCore*m_core;

	TimePrinter*m_timePrinter;

	SlaveMode RAY_SLAVE_MODE_NEIGHBOURHOOD;

	MasterMode RAY_MASTER_MODE_KILL_RANKS;
	MasterMode RAY_MASTER_MODE_NEIGHBOURHOOD;

	MessageTag RAY_MPI_TAG_NEIGHBOURHOOD;


	Adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD m_adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD;
	Adapter_RAY_MASTER_MODE_NEIGHBOURHOOD m_adapter_RAY_MASTER_MODE_NEIGHBOURHOOD;

	/** contig paths */
	vector<vector<Kmer> >*m_contigs;
	vector<uint64_t>*m_contigNames;

public:

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

	void call_RAY_SLAVE_MODE_NEIGHBOURHOOD();
	void call_RAY_MASTER_MODE_NEIGHBOURHOOD();
};

#endif
