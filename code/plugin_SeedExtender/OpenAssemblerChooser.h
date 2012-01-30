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

#ifndef _OpenAssemblerChooser
#define _OpenAssemblerChooser

#include <plugin_SeedExtender/Chooser.h> // for IMPOSSIBLE_CHOICE
#include <plugin_SeedExtender/NovaEngine.h>

/**
 * de Bruijn heuristic to choose extension direction in a graph, described in paper 
 * 	
 * 	Ray: simultaneous assembly of reads from a mix of high-throughput sequencing technologies.
 * 	Sébastien Boisvert, François Laviolette, and Jacques Corbeil.
 * 	Journal of Computational Biology (Mary Ann Liebert, Inc. publishers).
 * 	November 2010, 17(11): 1519-1533.
 * 	doi:10.1089/cmb.2009.0238
 * 	http://dx.doi.org/doi:10.1089/cmb.2009.0238
 * \author Sébastien Boisvert
 */
class OpenAssemblerChooser{
	NovaEngine m_novaEngine;

	double m_singleEndMultiplicator;
	double m_pairedEndMultiplicator;
	void updateMultiplicators();

	int getWinner(vector<set<int> >*battleVictories,int choices);
	
	/**
 * choose where to go based on coverage
 */
	void chooseWithCoverage(ExtensionData*ed,int minCoverage,vector<set<int> >*battleVictories);
public:
	int choose(ExtensionData*m_ed,Chooser*m_c,int m_minimumCoverage,
	Parameters*parameters);

	void constructor();

};

#endif
