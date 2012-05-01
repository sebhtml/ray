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

 /* generated_automatically */
class GeneOntology; /* generated_automatically */
 /* generated_automatically */
____CreateMasterModeAdapterDeclaration(GeneOntology,RAY_MASTER_MODE_ONTOLOGY_MAIN); /* generated_automatically */
 /* generated_automatically */
____CreateSlaveModeAdapterDeclaration(GeneOntology,RAY_SLAVE_MODE_ONTOLOGY_MAIN); /* generated_automatically */
 /* generated_automatically */
____CreateMessageTagAdapterDeclaration(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS); /* generated_automatically */
____CreateMessageTagAdapterDeclaration(GeneOntology,RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY); /* generated_automatically */
 /* generated_automatically */


/** 
 * a plugin to know what is present in a sample 
 * in terms of functions
 * \author Sébastien Boisvert
 */
class GeneOntology: public CorePlugin{

	char*m_ontologyFileName;
	char*m_annotationFileName;

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
	Adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS;
	Adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY m_adapter_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY;

	Adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN m_adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN;
	Adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN m_adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN;

	void fetchRelevantColors /**/ ();
	void loadAnnotations();
	bool fetchArguments();

	map<GeneOntologyIdentifier,map<COVERAGE_TYPE,int> > m_ontologyTermFrequencies;
	map<GeneOntologyIdentifier,map<COVERAGE_TYPE,int> >::iterator m_ontologyTermFrequencies_iterator1;
	map<COVERAGE_TYPE,int>::iterator m_ontologyTermFrequencies_iterator2;

	void countOntologyTermsInGraph();
	bool m_countOntologyTermsInGraph;

	// synchronization
	bool m_synced;
	void synchronize();
	void __skipToData();
	bool hasDataToSync();
	void addDataToBuffer(uint64_t*buffer,int*bufferPosition);
	void incrementOntologyTermFrequency(GeneOntologyIdentifier term,COVERAGE_TYPE kmerCoverage,int frequency);
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

public:

	void call_RAY_MASTER_MODE_ONTOLOGY_MAIN();
	void call_RAY_SLAVE_MODE_ONTOLOGY_MAIN();

	void call_RAY_MPI_TAG_SYNCHRONIZE_TERMS(Message*message);
	void call_RAY_MPI_TAG_SYNCHRONIZE_TERMS_REPLY(Message*message);


	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

};

#endif

