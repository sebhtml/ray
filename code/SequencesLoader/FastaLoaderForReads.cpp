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

#include "FastaLoaderForReads.h"

#include <code/Mock/constants.h>

#include <fstream>
#include <stdlib.h>
using namespace std;

int FastaLoaderForReads::open(string file){
	return m_fastqLoader.openWithPeriod(file,2);
}

void FastaLoaderForReads::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	m_fastqLoader.loadWithPeriod(maxToLoad,reads,seqMyAllocator,2);
}

int FastaLoaderForReads::getSize(){
	return m_fastqLoader.getSize();
}

void FastaLoaderForReads::close(){
	m_fastqLoader.close();
}
