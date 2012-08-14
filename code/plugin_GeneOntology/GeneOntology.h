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

#ifndef _GeneOntology_h
#define _GeneOntology_h

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
#include <plugin_GeneOntology/types.h>

#include <set>
#include <stdint.h>
#include <map>
#include <vector>
using namespace std;


/** 
 * a plugin to know what is present in a sample 
 * in terms of functions
 * \author Sébastien Boisvert
 */
class GeneOntology: public CorePlugin{

	string m_ontologyFileName;
	string m_annotationFileName;

	map<PhysicalKmerColor,vector<GeneOntologyIdentifier> > m_annotations;
	bool m_slaveStarted;

	bool m_loadedAnnotations;
	bool m_listedRelevantColors;

	set<PhysicalKmerColor> m_colorsForOntology;

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
	MasterMode RAY_MASTER_MODE_KILL_RANKS;
	MasterMode RAY_MASTER_MODE_NEIGHBOURHOOD;

	SlaveMode RAY_SLAVE_MODE_ONTOLOGY_MAIN;

	MessageTag RAY_MPI_TAG_ONTOLOGY_MAIN;
	MessageTag RAY_MPI_TAG_SYNCHRONIZE_TERMS;
	MessageTag RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY;
	MessageTag RAY_MPI_TAG_SYNCHRONIZATION_DONE;



	void fetchRelevantColors /**/ ();
	void loadAnnotations();
	bool fetchArguments();

	map<GeneOntologyIdentifier,map<CoverageDepth,int> > m_ontologyTermFrequencies;
	map<GeneOntologyIdentifier,map<CoverageDepth,int> >::iterator m_ontologyTermFrequencies_iterator1;
	map<CoverageDepth,int>::iterator m_ontologyTermFrequencies_iterator2;

	void countOntologyTermsInGraph();
	bool m_countOntologyTermsInGraph;

	// synchronization
	bool m_synced;
	void synchronize();
	void __skipToData();
	bool hasDataToSync();
	void addDataToBuffer(MessageUnit*buffer,int*bufferPosition);
	void incrementOntologyTermFrequency(GeneOntologyIdentifier term,CoverageDepth kmerCoverage,int frequency);
	bool m_waitingForReply;

	// write files
	void writeOntologyFiles();

	// load ontology
	void loadOntology(map<GeneOntologyIdentifier,string>*identifiers,
		map<GeneOntologyIdentifier,string>*descriptions);

	// load ontology names
	map<GeneOntologyIdentifier,string> m_identifiers;
	map<GeneOntologyIdentifier,string> m_descriptions;

	string getGeneOntologyName(GeneOntologyIdentifier handle);
	string getGeneOntologyIdentifier(GeneOntologyIdentifier handle);

	bool m_gotGeneOntologyParameter;

	// load parents from gene ontology file
	map<GeneOntologyIdentifier,vector<GeneOntologyIdentifier> > m_parents;
	bool hasParent(GeneOntologyIdentifier handle);
	void getParents(GeneOntologyIdentifier handle,vector<GeneOntologyIdentifier>*parents);
	void addParentGeneOntologyIdentifier(GeneOntologyIdentifier handle,GeneOntologyIdentifier parent);

	// get paths from root
	void getPathsFromRoot(GeneOntologyIdentifier handle,vector<vector<GeneOntologyIdentifier> >*paths);
	void printPathsFromRoot(GeneOntologyIdentifier handle,ostream*stream);

	// domains
	
	void setDomain(GeneOntologyIdentifier handle,GeneOntologyDomain domain);
	GeneOntologyDomain getGeneOntologyDomain(const char*text);

	map<GeneOntologyIdentifier,GeneOntologyDomain> m_termDomains;
	map<string,GeneOntologyDomain> m_domains;

	// tree processing
	
	GeneOntologyIdentifier m_biologicalProcessHandle;
	GeneOntologyIdentifier m_cellularComponentHandle;
	GeneOntologyIdentifier m_molecularFunctionHandle;
	map<GeneOntologyIdentifier,int> m_termCounts;
	map<GeneOntologyIdentifier,int> m_recursiveCounts;
	map<GeneOntologyIdentifier,vector<GeneOntologyIdentifier> > m_children;
	void getChildren(GeneOntologyIdentifier handle,vector<GeneOntologyIdentifier>*children);
	void addRecursiveCount(GeneOntologyIdentifier handle,int count);
	int getRecursiveCount(GeneOntologyIdentifier handle);
	void writeOntologyProfile(GeneOntologyDomain domain);
	int getDomainDepth(GeneOntologyDomain domain);
	int getDeepestDepth(GeneOntologyIdentifier handle,int depth);
	void writeTrees();
	void populateRecursiveValues();
	GeneOntologyDomain getDomain(GeneOntologyIdentifier handle);
	string getDomainName(GeneOntologyDomain handle);
	map<GeneOntologyDomain,string> m_domainNames;

	int computeRecursiveCount(GeneOntologyIdentifier handle,set<GeneOntologyIdentifier>*visited);
	int getGeneOntologyCount(GeneOntologyIdentifier handle);
	map<GeneOntologyIdentifier,int> m_depths;
	int getGeneOntologyDepth(GeneOntologyIdentifier handle);
	void computeDepths(GeneOntologyIdentifier root,int depth);
	bool hasDepth(GeneOntologyIdentifier handle);

	// alternate identifiers
	
	GeneOntologyIdentifier dereferenceTerm_safe(GeneOntologyIdentifier handle,set<GeneOntologyIdentifier>*visited);
	GeneOntologyIdentifier dereferenceTerm(GeneOntologyIdentifier handle);
	map<GeneOntologyIdentifier,GeneOntologyIdentifier> m_symbolicLinks;

	// dereferences
	LargeCount m_dereferences;

	// new formula for counting profile percentages
	LargeCount m_kmerObservationsWithGeneOntologies;
	bool m_synchronizedTotal;
	int m_ranksSynchronized;

public:

	void call_RAY_MASTER_MODE_ONTOLOGY_MAIN();
	void call_RAY_SLAVE_MODE_ONTOLOGY_MAIN();

	void call_RAY_MPI_TAG_SYNCHRONIZE_TERMS(Message*message);
	void call_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY(Message*message);

	void call_RAY_MPI_TAG_SYNCHRONIZATION_DONE(Message*message);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

};

#endif

