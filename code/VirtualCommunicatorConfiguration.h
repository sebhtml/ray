/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

/*
 * configuration of the virtual communicator
 */



Set( RAY_MPI_TAG_WRITE_CONTIG, 			3 )
Set( RAY_MPI_TAG_REQUEST_VERTEX_READS, 		4+KMER_U64_ARRAY_SIZE )
Set( RAY_MPI_TAG_GET_READ_MATE, 		4 )
Set( RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,	KMER_U64_ARRAY_SIZE )
Set( RAY_MPI_TAG_ATTACH_SEQUENCE,		KMER_U64_ARRAY_SIZE+4 )
Set( RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,	min(2,max(2,KMER_U64_ARRAY_SIZE)))
Set( RAY_MPI_TAG_HAS_PAIRED_READ,		1 )
Set( RAY_MPI_TAG_GET_READ_MARKERS,		5 )
Set( RAY_MPI_TAG_GET_PATH_LENGTH,		1 )
Set( RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,	4 )
Set( RAY_MPI_TAG_SCAFFOLDING_LINKS, 		6 )
Set( RAY_MPI_TAG_CONTIG_INFO,			2)
Set( MPI_TAG_CHECK_VERTEX,			KMER_U64_ARRAY_SIZE+1)

