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

#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>

/**
 * register the plugin
 * */
void GenomeNeighbourhood::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"GenomeNeighbourhood");
	core->setPluginDescription(m_plugin,"Get a sophisticated bird's-eye view of a sample's DNA");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3 (GPLv3)");

}

/**
 * resolve symbols
 */
void GenomeNeighbourhood::resolveSymbols(ComputeCore*core){

}

