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

#ifndef _VertexData
#define _VertexData

#include <stdint.h>
#include <ReadAnnotation.h>
#include <Direction.h>
#include <vector>
using namespace std;

class VertexData{
public:
	uint64_t m_lowerKey;
/*
 * 	read annotations
 * 	which reads start here?
 */
	ReadAnnotation*m_readsStartingHere;

/*
 *	which hyperfusions go on this vertex at least once?
 */
	Direction*m_directions;

	void constructor();
	void addRead(uint64_t a,ReadAnnotation*e);
	ReadAnnotation*getReads(uint64_t a);
	void addDirection(uint64_t a,Direction*d);
	vector<Direction> getDirections(uint64_t a);
	void clearDirections(uint64_t a);
}
#ifdef FORCE_PACKING
__attribute__((__packed__))
#endif
;

#endif
