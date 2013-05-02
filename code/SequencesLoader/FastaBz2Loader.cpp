/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
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

#ifdef CONFIG_HAVE_LIBBZ2

#include "FastaBz2Loader.h"

#include <stdlib.h>
#include <fstream>
using namespace std;

FastaBz2Loader::FastaBz2Loader() {
	addExtension(".fa.bz2");
	addExtension(".fasta.bz2");
}

int FastaBz2Loader::getSize(){
	return m_fastqBz2Loader.getSize();
}

int FastaBz2Loader::open(string file){
	return m_fastqBz2Loader.openWithPeriod(file,2);
}

void FastaBz2Loader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	m_fastqBz2Loader.loadWithPeriod(maxToLoad,reads,seqMyAllocator,2);
}

void FastaBz2Loader::close(){
	m_fastqBz2Loader.close();
}

#endif
