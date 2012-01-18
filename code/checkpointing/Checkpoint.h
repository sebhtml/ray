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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Checkpoint
#define _Checkpoint

/* Ray checkpointing files should have this header:

 magic number (8 bytesm uint64_t)
 k-mer length (4 bytes, int)
 KMER_U64_ARRAY_SIZE (4 bytes, int)
 Number of ranks (4 bytes, int)
 checkpoint type (4 bytes, int)
 checkpoint content (the rest)

for now, I don't use these in checkpointing files 
*/

#define RAY_CHECKPOINT_FILE_MAGIC_NUMBER 0xce8e612800a9a266

enum{
RAY_CHECKPOINT_PARTITION,
RAY_CHECKPOINT_COVERAGE_DISTRIBUTION,
RAY_CHECKPOINT_GENOME_GRAPH,
RAY_CHECKPOINT_OPTIMAL_MARKERS,
RAY_CHECKPOINT_OFFSETS,
RAY_CHECKPOINT_SEEDS,
RAY_CHECKPOINT_EXTENSIONS
};

#endif

