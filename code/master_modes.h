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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/




#ifndef _master_modes
#define _master_modes

// master modes
#define MASTER_MODE_LOAD_CONFIG 0x0
#define MASTER_MODE_LOAD_SEQUENCES 0x1
#define MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION 0x2
#define MASTER_MODE_SEND_COVERAGE_VALUES 0x3
#define MASTER_MODE_TRIGGER_EDGES_DISTRIBUTION 0x4
#define MASTER_MODE_START_EDGES_DISTRIBUTION 0x5
#define MASTER_MODE_DO_NOTHING 0x6
#define MASTER_MODE_UPDATE_DISTANCES 0x7
#define MASTER_MODE_ASK_EXTENSIONS 0x8
#define MASTER_MODE_AMOS 0x9
#define MASTER_MODE_ASSEMBLE_GRAPH 0xa
#define MASTER_MODE_PREPARE_DISTRIBUTIONS 0xb
#define MASTER_MODE_TRIGGER_EDGES 0xc
#define MASTER_MODE_TRIGGER_INDEXING 0xd
#define MASTER_MODE_INDEX_SEQUENCES 			0xe
#define MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS 	0xf
#define MASTER_MODE_PREPARE_SEEDING 			0x10
#define MASTER_MODE_TRIGGER_SEEDING			0x11
#define MASTER_MODE_TRIGGER_DETECTION			0x12
#define MASTER_MODE_ASK_DISTANCES 			0x13
#define MASTER_MODE_START_UPDATING_DISTANCES		0x14
#define MASTER_MODE_TRIGGER_EXTENSIONS			0x15
#define MASTER_MODE_TRIGGER_FUSIONS 			0x16
#define MASTER_MODE_TRIGGER_FIRST_FUSIONS 		0x17
#define MASTER_MODE_START_FUSION_CYCLE 			0x18


#endif
