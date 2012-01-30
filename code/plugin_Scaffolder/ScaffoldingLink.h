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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _ScaffoldingLink_h
#define _ScaffoldingLink_h

/**
 * A scaffolding link is a potentially useful link between two contigs
 * \author Sébastien Boisvert
 */
class ScaffoldingLink{
	/* the distance between the 2 contigs */
	int m_distance;

	int m_path1MarkerCoverage;

	int m_path2MarkerCoverage;
public:
	ScaffoldingLink();
	void constructor(int distance,int coverage1,int coverage2);
	int getDistance();
	int getCoverage1();
	int getCoverage2();
};

#endif

