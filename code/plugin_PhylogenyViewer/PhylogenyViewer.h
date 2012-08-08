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

1. for each sequence, also add an extra color for its genome identifier using a distinct namespace  [DONE]
2. Color things [DONE]

3. Use the vertices to get a list of identifiers. [DONE]
4. With this list, load only the relevant pairs from Genome-to-Taxon.tsv. (use an iterator)  [DONE]
5. generate a list of relevant taxon identifiers  [DONE]

6. synchronize taxons [DONE]

7. iteratively load the tree of life (using an iterator-like approach) and fetch things to complete paths to root [DONE]

8. load taxon names [DONE]

9. For each vertex, get the best guess in the tree
	for instance if a k-mer has 3 things on it, try to find a common ancestor in the tree [DONE]

10. also add a Unknown category, which are the k-mers without colors but assembled de novo [DONE]

11. synchronize the tree with master [DONE]

12. output BiologicalAbundances/_Phylogeny/Hits.txt [DONE]

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
#include <profiling/TimePrinter.h>

#include <plugin_Searcher/Searcher.h>
#include <plugin_PhylogenyViewer/types.h>

#include <set>
#include <stdint.h>
#include <map>
using namespace std;


/** 
 * a plugin to know what is present in a sample 
 * \author Sébastien Boisvert
 */
class PhylogenyViewer: public CorePlugin{

/* slave states */
	bool m_extractedColorsForPhylogeny;
	bool m_loadedTaxonsForPhylogeny;
	bool m_sentTaxonsToMaster;
	bool m_sentTaxonControlMessage;
	bool m_messageSent;
	bool m_messageReceived;
	bool m_synced;
	bool m_loadedTree;
	bool m_gatheredObservations;
	bool m_syncedTree;
	bool m_unknownSent;

	map<TaxonIdentifier,LargeCount>::iterator m_countIterator;

	set<TaxonIdentifier>::iterator m_taxonIterator;

/* master states */

	int m_ranksThatLoadedTaxons;
	bool m_mustSync;
	int m_responses;

	set<GenomeIdentifier> m_warnings;

	set<PhysicalKmerColor> m_colorsForPhylogeny;
	set<TaxonIdentifier> m_taxonsForPhylogeny;
	set<TaxonIdentifier> m_taxonsForPhylogenyMaster;
	map<GenomeIdentifier,TaxonIdentifier> m_genomeToTaxon;

	map<TaxonIdentifier,string> m_taxonNames;
	map<TaxonIdentifier,string> m_taxonRanks;

	map<TaxonIdentifier,LargeCount> m_taxonObservations;
	map<TaxonIdentifier,LargeCount> m_taxonObservationsMaster;

	map<TaxonIdentifier,LargeCount> m_taxonRecursiveObservations;

	TaxonIdentifier UNKNOWN_TAXON;

	LargeCount m_unknown;
	LargeCount m_unknownMaster;

	map<TaxonIdentifier,set<TaxonIdentifier> > m_treeChildren;
	map<TaxonIdentifier,TaxonIdentifier> m_treeParents;

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
	TimePrinter*m_timePrinter;

	ComputeCore*m_core;

	bool m_started;

	Searcher*m_searcher;

	MasterMode RAY_MASTER_MODE_ONTOLOGY_MAIN;
	MasterMode RAY_MASTER_MODE_PHYLOGENY_MAIN;
	MasterMode RAY_MASTER_MODE_KILL_RANKS;
	MasterMode RAY_MASTER_MODE_NEIGHBOURHOOD;

	SlaveMode RAY_SLAVE_MODE_PHYLOGENY_MAIN;

	MessageTag RAY_MPI_TAG_PHYLOGENY_MAIN;
	MessageTag RAY_MPI_TAG_TOUCH_TAXON;
	MessageTag RAY_MPI_TAG_TOUCH_TAXON_REPLY;
	MessageTag RAY_MPI_TAG_SYNCED_TAXONS;
	MessageTag RAY_MPI_TAG_LOADED_TAXONS;
	MessageTag RAY_MPI_TAG_TAXON_OBSERVATIONS;
	MessageTag RAY_MPI_TAG_TAXON_OBSERVATIONS_REPLY;


	bool m_loadAllTree;

	LargeCount m_totalNumberOfKmerObservations;

	void extractColorsForPhylogeny();
	void loadTaxons();

	void sendTaxonsToMaster();
	void sendTaxonsFromMaster();
	void copyTaxonsFromSecondaryTable();
	void loadTree();
	
	void testPaths();

	void getTaxonPathFromRoot(TaxonIdentifier taxon,vector<TaxonIdentifier>*path);

	void loadTaxonNames();

	string getTaxonName(TaxonIdentifier taxon);

	void gatherKmerObservations();
	void classifySignal(vector<TaxonIdentifier>*taxons,int kmerCoverage,Vertex*vertex,Kmer*key);
	void printTaxonPath(TaxonIdentifier taxon,vector<TaxonIdentifier>*path,ostream*stream);
	TaxonIdentifier getTaxonParent(TaxonIdentifier taxon);
	void showObservations(ostream*stream);
	void sendTreeCounts();
	TaxonIdentifier findCommonAncestor(vector<TaxonIdentifier>*taxons);
	string getTaxonRank(TaxonIdentifier taxon);

	void showObservations_XML(ostream*stream);
	void printTaxonPath_XML(TaxonIdentifier taxon,vector<TaxonIdentifier>*path,ostream*stream);
	void printTaxon_XML(TaxonIdentifier taxon,ostream*stream);
	LargeCount getRecursiveCount(TaxonIdentifier taxon);

	void populateRanks(map<string,LargeCount>*rankSelfObservations,
		map<string,LargeCount>*rankRecursiveObservations);
	LargeCount getSelfCount(TaxonIdentifier taxon);

public:

	void call_RAY_MASTER_MODE_PHYLOGENY_MAIN();
	void call_RAY_SLAVE_MODE_PHYLOGENY_MAIN();
	void call_RAY_MPI_TAG_TOUCH_TAXON(Message*message);
	void call_RAY_MPI_TAG_TAXON_OBSERVATIONS(Message*message);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

};

#endif

