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

__CreatePlugin(PathEvaluator);

__CreateMasterModeAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS);

void PathEvaluator::call_RAY_MASTER_MODE_EVALUATE_PATHS(){
	m_core->getSwitchMan()->closeMasterMode();
}

void PathEvaluator::registerPlugin(ComputeCore*core){

	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;
	m_core=core;

	core->setPluginName(plugin,"PathEvaluator");
	core->setPluginDescription(plugin,"Post-processing of paths.");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_EVALUATE_PATHS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_EVALUATE_PATHS,
		__GetAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_EVALUATE_PATHS,"RAY_MASTER_MODE_EVALUATE_PATHS");
}

void PathEvaluator::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_EVALUATE_PATHS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_EVALUATE_PATHS");
	RAY_MASTER_MODE_ASK_EXTENSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ASK_EXTENSIONS");

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_EVALUATE_PATHS,RAY_MASTER_MODE_ASK_EXTENSIONS);

	__BindPlugin(PathEvaluator);

	__BindAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS);
}
