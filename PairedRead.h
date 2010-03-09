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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _PairedRead
#define _PairedRead

/*
 *    LeftRead							RightRead
 * ------------>
 *                                                       <-----------
 *
 *                   AverageFragmentLength  +/- StandardDeviation
 * <----------------------------------------------------------------->
 */
class PairedRead{
	int m_rank;
	int m_sequence_id;
	int m_fragmentSize;
	int m_deviation;
public:
	PairedRead();
	void constructor(int rank,int id, int fragmentSize,int deviation);
	int getRank();
	int getId();
	int getAverageFragmentLength();
	int getStandardDeviation();
};

#endif
