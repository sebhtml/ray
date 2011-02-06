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

#ifndef _GridTableIterator
#define _GridTableIterator

#include <GridTable.h>

class GridTableIterator{
	GridTable*m_table;
	int m_currentBin;
	int m_currentPosition;
	void getNext();
public:
	void constructor(GridTable*a);
	bool hasNext();
	GridData*next();
};

#endif
