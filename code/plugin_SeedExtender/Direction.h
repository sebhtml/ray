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

#ifndef _Direction
#define _Direction

#include <stdint.h>
#include <application_core/common_functions.h>

/*
 * a direction is a wave and a progression.
 *
 * A wave is the flow given by the Parallel_Ray_Engine in the graph.
 * directions of the flow are stored with Directions. (as linked lists).
 * \author Sébastien Boisvert
 */
class Direction{
	Direction*m_next;
	PathHandle m_wave; // the path Identifier, as a wave for itself.
	uint32_t m_progression; // the position in the path.
	bool m_lower;
public:
	void constructor(PathHandle wave,int progression,bool lower);
	PathHandle getWave();
	int getProgression();
	Direction*getNext();
	void setNext(Direction*e);
	bool isLower();
} ATTRIBUTE_PACKED;

#endif

