/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

*/

// the Ray engine will run these master modes in sequence
// for those who support the feature.

MACRO_LIST_ITEM( RAY_MASTER_MODE_TEST_NETWORK )
MACRO_LIST_ITEM( RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS )

/* sequence loading */
MACRO_LIST_ITEM( RAY_MASTER_MODE_COUNT_FILE_ENTRIES )
MACRO_LIST_ITEM( RAY_MASTER_MODE_LOAD_SEQUENCES )



MACRO_LIST_ITEM( RAY_MASTER_MODE_WRITE_KMERS )
MACRO_LIST_ITEM( RAY_MASTER_MODE_TRIGGER_INDEXING )

/* seed computation */
MACRO_LIST_ITEM( RAY_MASTER_MODE_PREPARE_SEEDING )

MACRO_LIST_ITEM( RAY_MASTER_MODE_TRIGGER_SEEDING )

/* compute the distances between reads of all pairs */
MACRO_LIST_ITEM( RAY_MASTER_MODE_START_UPDATING_DISTANCES )
MACRO_LIST_ITEM( RAY_MASTER_MODE_UPDATE_DISTANCES )

/* simplify the assembly as much as possible */
MACRO_LIST_ITEM( RAY_MASTER_MODE_TRIGGER_FUSIONS )
MACRO_LIST_ITEM( RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS )

MACRO_LIST_ITEM( RAY_MASTER_MODE_START_FUSION_CYCLE )

// some modes are not yet ported to the scripting engine

MACRO_LIST_ITEM( RAY_MASTER_MODE_ASK_EXTENSIONS )
MACRO_LIST_ITEM( RAY_MASTER_MODE_SCAFFOLDER )
MACRO_LIST_ITEM( RAY_MASTER_MODE_WRITE_SCAFFOLDS )

// the last steps
MACRO_LIST_ITEM( RAY_MASTER_MODE_KILL_RANKS )
MACRO_LIST_ITEM( RAY_MASTER_MODE_KILL_ALL_MPI_RANKS )
