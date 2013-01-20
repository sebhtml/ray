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

LoaderInterface*LoaderFactory::makeLoader(string file){
	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() &&
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		m_type=FORMAT_CSFASTA;
		return &m_color;
	}
	if(file.substr(file.length()-4,4)==".sff"){
		m_type=FORMAT_SFF;
		return &m_sff;
		
	}

	if(file.substr(file.length()-6,6)==".fasta"){
		m_type=FORMAT_FASTA;
		return &m_fastaLoader;
	}

	if(file.substr(file.length()-10,10)=="export.txt"){
		m_type=FORMAT_EXPORT;
		return &m_export;
	}

	if(file.substr(file.length()-6,6)==".fastq"){
		m_type=FORMAT_FASTQ;
		return &m_fastq;
	}

	#ifdef CONFIG_HAVE_LIBZ
	if(file.substr(file.length()-9,9)==".fastq.gz"){
		m_type=FORMAT_FASTQ_GZ;
		return &m_fastqgz;
	}

	if(file.substr(file.length()-9,9)==".fasta.gz"){
		m_type=FORMAT_FASTA_GZ;
		return &m_fastagz;
	}
	#endif

	#ifdef CONFIG_HAVE_LIBBZ2
	if(file.substr(file.length()-10,10)==".fastq.bz2"){
		m_type=FORMAT_FASTQ_BZ2;
		return &m_fastqbz2;
	}

	if(file.substr(file.length()-10,10)==".fasta.bz2"){
		m_type=FORMAT_FASTA_BZ2;
		return &m_fastabz2;
	}
	#endif

	return NULL;
}

