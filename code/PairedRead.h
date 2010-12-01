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

*/

#ifndef _PairedRead
#define _PairedRead

#include<common_functions.h>

/*
 *    LeftRead							RightRead
 * ------------>
 *                                                       <-----------
 *
 *                   AverageFragmentLength  +/- StandardDeviation
 * <----------------------------------------------------------------->
 */
class PairedRead{
	u32 m_readIndex;
	u16 m_rank;
	u8 m_library;
public:
	PairedRead();
	void constructor(int rank,int id,int library);
	int getRank();
	int getId();
	u64 getUniqueId();
	int getLibrary();
};

#endif
