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

#ifndef _Chooser
#define _Chooser

#define IMPOSSIBLE_CHOICE -1
#include<ExtensionData.h>
#include<ChooserData.h>

class Chooser{
public:
	int chooseWithPairedReads(ExtensionData*m_ed,ChooserData*m_cd,int m_minimumCoverage,int m_maxCoverage);
	void clear(int*a,int b);
};

#endif
