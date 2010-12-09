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

enum{

MODE_EXTENSION_ASK,
MODE_START_SEEDING,
MODE_DO_NOTHING,
MODE_ASK_EXTENSIONS,
MODE_SEND_EXTENSION_DATA,
MODE_ASSEMBLE_WAVES,
MODE_ASSEMBLE_GRAPH,
MODE_FUSION,
MODE_MASTER_ASK_CALIBRATION,
MODE_PERFORM_CALIBRATION,
MODE_FINISH_FUSIONS,
MODE_DISTRIBUTE_FUSIONS,
MODE_AMOS,
MODE_AUTOMATIC_DISTANCE_DETECTION,
MODE_SEND_LIBRARY_DISTANCES,
MODE_UPDATE_DISTANCES,
MODE_EXTRACT_VERTICES,
MODE_SEND_DISTRIBUTION,
MODE_PROCESS_INGOING_EDGES,
MODE_PROCESS_OUTGOING_EDGES,
MODE_EXTENSION,
MODE_INDEX_SEQUENCES

};

#endif
