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
*/

/*

[sboisver12@colosse1 2012-01-25]$ ls
Genome-to-Taxon.tsv  Taxon-Names.tsv  Taxon-Types.tsv  TreeOfLife-Edges.tsv

3. for each sequence, also add an extra color for its genome identifier using a distinct namespace  [DONE]
4. Color things [DONE]

1. Use the vertices to get a list of identifiers. [DONE]
2. With this list, load only the relevant pairs from Genome-to-Taxon.tsv. (use an iterator)  [DONE]
x. generate a list of relevant taxon identifiers  [DONE]

y. iteratively load the tree of life (using an iterator-like approach) and fetch things to complete paths to root

5. For each vertex, get the best guess in the tree
	for instance if a k-mer has 3 things on it, try to find a common ancestor in the tree
6. synchronize the tree with master
7. output BiologicalAbundances/_Phylogeny/Hits.tsv

also add a Unknown category, which are the k-mers without colors but assembled de novo

*/

#ifndef _PhylogenyViewer_h
#define _PhylogenyViewer_h

#include <core/ComputeCore.h>
#include <plugins/CorePlugin.h>
#include <handlers/SlaveModeHandler.h>
#include <plugin_Searcher/ColorSet.h>
#include <handlers/MasterModeHandler.h>
#include <handlers/MessageTagHandler.h>
#include <application_core/Parameters.h>
#include <plugin_VerticesExtractor/GridTable.h>


#include <plugin_PhylogenyViewer/PhylogenyViewer_adapters.h>

#include <set>
#include <stdint.h>
#include <map>
using namespace std;

/** 
 * a plugin to know what is present in a sample 
 * \author Sébastien Boisvert
 */
class PhylogenyViewer: public CorePlugin{

	bool m_extractedColorsForPhylogeny;
	bool m_loadedTaxonsForPhylogeny;

	set<PhysicalKmerColor> m_colorsForPhylogeny;
	set<uint64_t> m_taxonsForPhylogeny;
	map<uint64_t,uint64_t> m_genomeToTaxon;

	GridTable*m_subgraph;
	Parameters*m_parameters;
	SwitchMan*m_switchMan;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	ColorSet*m_colorSet;

	Rank m_rank;
	int m_size;

	StaticVector*m_inbox;
	RingAllocator*m_inboxAllocator;

	ComputeCore*m_core;

	bool m_started;

	MasterMode RAY_MASTER_MODE_PHYLOGENY_MAIN;
	MasterMode RAY_MASTER_MODE_KILL_RANKS;

	SlaveMode RAY_SLAVE_MODE_PHYLOGENY_MAIN;
	MessageTag RAY_MPI_TAG_PHYLOGENY_MAIN;

	Adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN m_adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN;
	Adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN m_adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN;
	
	void extractColorsForPhylogeny();
	void loadTaxons();

public:

	void call_RAY_MASTER_MODE_PHYLOGENY_MAIN();
	void call_RAY_SLAVE_MODE_PHYLOGENY_MAIN();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

};

#endif

