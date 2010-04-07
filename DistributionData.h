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

#ifndef _DistributionData
#define _DistributionData
#include<map>
#include<vector>
#include<common_functions.h>

class DistributionData{
public:
	std::map<int,std::vector<VERTEX_TYPE> > m_messagesStock;
	std::map<int,std::vector<VERTEX_TYPE> > m_messagesStockOut;
	std::map<int,std::vector<VERTEX_TYPE> > m_messagesStockIn;
	std::map<int,std::vector<VERTEX_TYPE> > m_attachedSequence;
	std::map<int,std::vector<VERTEX_TYPE> > m_messagesStockPaired;
};

#endif
