/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include "Mock.h"
#include "Parameters.h"
#include <code/Surveyor/Mother.h>

void Mock::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"Mock");
	core->setPluginDescription(m_plugin,"This plugin does nothing at all.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");
}

void Mock::resolveSymbols(ComputeCore*core){
	/* nothing to resolve... */

	m_core = core;
	Parameters * parameters=(Parameters*)m_core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");

	if(parameters->hasOption("-run-surveyor")) {
		Mother * mother = new Mother();

		mother->setParameters(parameters);
		m_core->spawnActor(mother);

		m_core->setActorModelOnly();
	}
}

