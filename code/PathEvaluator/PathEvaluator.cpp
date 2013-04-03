/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2013 Sébastien Boisvert

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

#include "PathEvaluator.h"

#include <fstream>
using namespace std;

__CreatePlugin(PathEvaluator);

__CreateMasterModeAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS);
__CreateSlaveModeAdapter(PathEvaluator,RAY_SLAVE_MODE_EVALUATE_PATHS);

void PathEvaluator::call_RAY_MASTER_MODE_EVALUATE_PATHS(){

	if(!m_masterModeStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_masterModeStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void PathEvaluator::call_RAY_SLAVE_MODE_EVALUATE_PATHS(){

	writeCheckpointForContigPaths();

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

void PathEvaluator::writeCheckpointForContigPaths(){

	/** possibly write the checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("ContigPaths")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint ContigPaths"<<endl;
		ofstream f(m_parameters->getCheckpointFile("ContigPaths").c_str());
		int theSize=m_contigs->size();
		f.write((char*)&theSize,sizeof(int));

		/* write each path with its name and vertices */
		for(int i=0;i<theSize;i++){
			PathHandle name=(*m_contigNames)[i];
			int vertices=(*m_contigs)[i].size();
			f.write((char*)&name,sizeof(PathHandle));
			f.write((char*)&vertices,sizeof(int));
			for(int j=0;j<vertices;j++){
				Kmer kmer;
				(*m_contigs)[i].at(j,&kmer);
				kmer.write(&f);
			}
		}
		f.close();
	}
}

void PathEvaluator::registerPlugin(ComputeCore*core){

	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;
	m_core=core;

	core->setPluginName(plugin,"PathEvaluator");
	core->setPluginDescription(plugin,"Post-processing of paths.");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	__ConfigureMasterModeHandler(PathEvaluator, RAY_MASTER_MODE_EVALUATE_PATHS);
	__ConfigureSlaveModeHandler(PathEvaluator, RAY_SLAVE_MODE_EVALUATE_PATHS);

	RAY_MPI_TAG_EVALUATE_PATHS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EVALUATE_PATHS,"RAY_MPI_TAG_EVALUATE_PATHS");

	__BindPlugin(PathEvaluator);
}

void PathEvaluator::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_EVALUATE_PATHS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_EVALUATE_PATHS");
	RAY_MASTER_MODE_ASK_EXTENSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ASK_EXTENSIONS");

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_EVALUATE_PATHS,RAY_MASTER_MODE_ASK_EXTENSIONS);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_EVALUATE_PATHS,RAY_MPI_TAG_EVALUATE_PATHS);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_EVALUATE_PATHS,RAY_SLAVE_MODE_EVALUATE_PATHS);

	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_contigs=(vector<GraphPath>*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigPaths.ray");
	m_contigNames=(vector<PathHandle>*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigNames.ray");

	m_masterModeStarted=false;
}
