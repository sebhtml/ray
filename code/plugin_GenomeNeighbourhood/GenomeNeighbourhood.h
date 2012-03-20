/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

\file GenomeNeighbourhood.h
\author Sébastien Boisvert
*/


#ifndef _GenomeNeighbourhood_h
#define _GenomeNeighbourhood_h

#include <plugins/CorePlugin.h>
#include <core/ComputeCore.h>
#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood_adapters.h>

/**
 * The plugin GenomeNeighbourhood outputs a file file
 * containing contig neighbourhoods.
 *
 * This is useful to know where is located a drug-resistance gene,
 * amongst other things.
 *
 * \author Sébastien Boisvert
 * \
 * */
class GenomeNeighbourhood: public CorePlugin{
public:

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);


};

#endif
