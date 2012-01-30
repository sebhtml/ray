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

#ifndef _BubbleTool
#define _BubbleTool

#include <vector>
#include <application_core/common_functions.h>
#include <map>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <application_core/Parameters.h>
using namespace std;

/*
 * Check if a bubble is genuine or not.
 * Data is stored in BubbleData
 * TODO: BubbleData and BubbleTool should me merged.
 * \author Sébastien Boisvert
 */
class BubbleTool{
	Parameters*m_parameters;
	Kmer m_choice;
public:
	bool isGenuineBubble(Kmer root, vector<vector<Kmer > >*trees,
map<Kmer ,int>*coverages,int repeatCoverage);
	void constructor(Parameters*p);

	Kmer getTraversalStartingPoint();

	void printStuff(Kmer root, vector<vector<Kmer > >*trees,
map<Kmer ,int>*coverages);
};

#endif

