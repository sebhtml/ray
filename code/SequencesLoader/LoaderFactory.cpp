/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "LoaderFactory.h"

LoaderFactory::LoaderFactory() {

	m_loaders.push_back(&m_color);
	m_loaders.push_back(&m_sff);
	m_loaders.push_back(&m_fastaLoader);
	m_loaders.push_back(&m_export);
	m_loaders.push_back(&m_fastq);

	#ifdef CONFIG_HAVE_LIBZ
	m_loaders.push_back(&m_fastqgz);
	m_loaders.push_back(&m_fastagz);
	#endif

	#ifdef CONFIG_HAVE_LIBBZ2
	m_loaders.push_back(&m_fastqbz2);
	m_loaders.push_back(&m_fastabz2);
	#endif
}

LoaderInterface* LoaderFactory::makeLoader(string fileName) {
	vector<LoaderInterface*>::iterator iterator;
	for(iterator = m_loaders.begin(); iterator != m_loaders.end(); ++iterator) {
		if((*iterator)->checkFileType(fileName.c_str())) {
			return *iterator;
		}
	}
	return NULL;
}
