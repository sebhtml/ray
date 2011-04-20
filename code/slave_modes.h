/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _slave_modes
#define _slave_modes

// slave modes

extern const char* SLAVE_MODES[];

enum{
RAY_SLAVE_MODE_LOAD_SEQUENCES,
RAY_SLAVE_MODE_EXTENSION_ASK,
RAY_SLAVE_MODE_START_SEEDING,
RAY_SLAVE_MODE_DO_NOTHING,
RAY_SLAVE_MODE_ASK_EXTENSIONS,
RAY_SLAVE_MODE_SEND_EXTENSION_DATA,
RAY_SLAVE_MODE_ASSEMBLE_WAVES,
RAY_SLAVE_MODE_ASSEMBLE_GRAPH,
RAY_SLAVE_MODE_FUSION,
RAY_SLAVE_MODE_FINISH_FUSIONS,
RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS,
RAY_SLAVE_MODE_AMOS,
RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION,
RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES,
RAY_SLAVE_MODE_UPDATE_DISTANCES,
RAY_SLAVE_MODE_EXTRACT_VERTICES,
RAY_SLAVE_MODE_SEND_DISTRIBUTION,
RAY_SLAVE_MODE_PROCESS_INGOING_EDGES,
RAY_SLAVE_MODE_PROCESS_OUTGOING_EDGES,
RAY_SLAVE_MODE_EXTENSION,
RAY_SLAVE_MODE_INDEX_SEQUENCES,
RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION,
RAY_SLAVE_MODE_DELETE_VERTICES,
RAY_SLAVE_MODE_DUMMY
};

#endif
