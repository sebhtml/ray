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

#ifndef _master_modes
#define _master_modes

// master modes

enum{

MASTER_MODE_LOAD_CONFIG,
MASTER_MODE_LOAD_SEQUENCES,
MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION,
MASTER_MODE_SEND_COVERAGE_VALUES,
MASTER_MODE_TRIGGER_EDGES_DISTRIBUTION,
MASTER_MODE_START_EDGES_DISTRIBUTION,
MASTER_RAY_SLAVE_MODE_DO_NOTHING,
MASTER_RAY_SLAVE_MODE_UPDATE_DISTANCES,
MASTER_RAY_SLAVE_MODE_ASK_EXTENSIONS,
MASTER_RAY_SLAVE_MODE_AMOS,
MASTER_RAY_SLAVE_MODE_ASSEMBLE_GRAPH,
MASTER_MODE_PREPARE_DISTRIBUTIONS,
MASTER_MODE_TRIGGER_EDGES,
MASTER_MODE_TRIGGER_INDEXING,
MASTER_RAY_SLAVE_MODE_INDEX_SEQUENCES,
MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS,
MASTER_MODE_PREPARE_SEEDING,
MASTER_MODE_TRIGGER_SEEDING,
MASTER_MODE_TRIGGER_DETECTION,
MASTER_MODE_ASK_DISTANCES,
MASTER_MODE_START_UPDATING_DISTANCES,
MASTER_MODE_TRIGGER_EXTENSIONS,
MASTER_MODE_TRIGGER_FUSIONS,
MASTER_MODE_TRIGGER_FIRST_FUSIONS,
MASTER_MODE_START_FUSION_CYCLE,
MASTER_MODE_ASK_BEGIN_REDUCTION,
MASTER_MODE_START_REDUCTION,
MASTER_MODE_RESUME_VERTEX_DISTRIBUTION

};

#endif
