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

#ifdef CONFIG_HAVE_LIBZ

#include "FastaGzLoader.h"

#include <fstream>
#include <stdlib.h>
using namespace std;

int FastaGzLoader::open(string file){
	return m_fastqGzLoader.openWithPeriod(file,2);
}

void FastaGzLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	m_fastqGzLoader.loadWithPeriod(maxToLoad,reads,seqMyAllocator,2);
}

int FastaGzLoader::getSize(){
	return m_fastqGzLoader.getSize();
}

void FastaGzLoader::close(){
	m_fastqGzLoader.close();
}

#endif

