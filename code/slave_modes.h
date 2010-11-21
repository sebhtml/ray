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



#ifndef _slave_modes
#define _slave_modes

// slave modes
#define MODE_EXTENSION_ASK 0x0
#define MODE_START_SEEDING 0x1
#define MODE_DO_NOTHING 0x2
#define MODE_ASK_EXTENSIONS 0x3
#define MODE_SEND_EXTENSION_DATA 0x4
#define MODE_ASSEMBLE_WAVES 0x5
#define MODE_COPY_DIRECTIONS 0x6
#define MODE_ASSEMBLE_GRAPH 0x7
#define MODE_FUSION 0x8
#define MODE_MASTER_ASK_CALIBRATION 0x9
#define MODE_PERFORM_CALIBRATION 0xa
#define MODE_FINISH_FUSIONS 0xb
#define MODE_DISTRIBUTE_FUSIONS 0xc
#define MODE_AMOS 0xd
#define MODE_AUTOMATIC_DISTANCE_DETECTION 0xe
#define MODE_SEND_LIBRARY_DISTANCES 0xf
#define MODE_UPDATE_DISTANCES 0x10
#define MODE_EXTRACT_VERTICES			0x11
#define MODE_SEND_DISTRIBUTION 			0x12
#define MODE_PROCESS_INGOING_EDGES 		0x13
#define MODE_PROCESS_OUTGOING_EDGES 		0x14
#define MODE_EXTENSION				0x15
#define MODE_INDEX_SEQUENCES                	0x16 // new!

#endif
