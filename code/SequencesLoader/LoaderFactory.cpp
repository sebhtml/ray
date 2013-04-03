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

/*
 * TODO: check if m_type is required, if not remove it.
 */
LoaderInterface*LoaderFactory::makeLoader(string file){
	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() &&
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		m_type=FORMAT_CSFASTA;
		return &m_color;
	}
	if(hasSuffix(file.c_str(),".sff")){
		m_type=FORMAT_SFF;
		return &m_sff;
	}

	if(hasSuffix(file.c_str(),".fasta")){
		m_type=FORMAT_FASTA;
		return &m_fastaLoader;
	}

	if(hasSuffix(file.c_str(),"export.txt")){
		m_type=FORMAT_EXPORT;
		return &m_export;
	}

	if(hasSuffix(file.c_str(),".fastq")){
		m_type=FORMAT_FASTQ;
		return &m_fastq;
	}

	#ifdef CONFIG_HAVE_LIBZ
	if(hasSuffix(file.c_str(),".fastq.gz")){
		m_type=FORMAT_FASTQ_GZ;
		return &m_fastqgz;
	}

	if(hasSuffix(file.c_str(),".fasta.gz")){
		m_type=FORMAT_FASTA_GZ;
		return &m_fastagz;
	}
	#endif

	#ifdef CONFIG_HAVE_LIBBZ2
	if(hasSuffix(file.c_str(),".fastq.bz2")){
		m_type=FORMAT_FASTQ_BZ2;
		return &m_fastqbz2;
	}

	if(hasSuffix(file.c_str(),".fasta.bz2")){
		m_type=FORMAT_FASTA_BZ2;
		return &m_fastabz2;
	}
	#endif

	return NULL;
}

bool LoaderFactory::hasSuffix(const char*fileName,const char*suffix){
	int fileNameLength=strlen(fileName);
	int suffixLength=strlen(suffix);

	if(suffixLength>fileNameLength)
		return false;

	int delta=0;

	while(delta<suffixLength){
		if(suffix[suffixLength-1-delta]!=fileName[fileNameLength-1-delta])
			return false;
		delta++;
	}

	return true;
}
