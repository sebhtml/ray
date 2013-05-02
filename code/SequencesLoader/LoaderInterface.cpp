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

#include "LoaderInterface.h"

bool LoaderInterface::hasSuffix(const char* fileName,const char*suffix) {
	int fileNameLength=strlen(fileName);
        int suffixLength=strlen(suffix);

        if(suffixLength>fileNameLength)
                return false;

        int delta=0;

        while(delta<suffixLength) {
                if(suffix[suffixLength-1-delta] != 
			fileName[fileNameLength-1-delta])
                        return false;
                delta++;
        }
        return true;
}

void LoaderInterface::addExtension(const char* extension) {
	m_extensions.push_back(extension);
}

bool LoaderInterface::checkFileType(const char* fileName) {
	vector<string>::iterator iterator;
	for(iterator = m_extensions.begin(); iterator != m_extensions.end(); ++iterator) {
		if(hasSuffix(fileName, iterator->c_str())) {
			return true;
		}
	}
	return false;
}
