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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _ChooserData
#define _ChooserData

class ChooserData{
public:
	// paired-end resolution of repeats.
	int m_CHOOSER_theMaxsPaired[4];
	int m_CHOOSER_theSumsPaired[4];
	int m_CHOOSER_theNumbersPaired[4];
	// single-end resolution of repeats.
	int m_CHOOSER_theMaxs[4];
	int m_CHOOSER_theSums[4];
	int m_CHOOSER_theNumbers[4];

};

#endif
