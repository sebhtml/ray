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

#ifndef _LoaderFactory_h
#define _LoaderFactory_h

#include "FastaLoader.h"
#include "FastaLoaderForReads.h"
#include "ExportLoader.h"
#include "FastqLoader.h"
#include "ColorSpaceLoader.h"
#include "SffLoader.h"

#ifdef CONFIG_HAVE_LIBZ
#include "FastqGzLoader.h"
#include "FastaGzLoader.h"
#endif

#ifdef CONFIG_HAVE_LIBBZ2
#include "FastqBz2Loader.h"
#include "FastaBz2Loader.h"
#endif

enum{
FORMAT_NULL,
FORMAT_CSFASTA,
FORMAT_SFF,
FORMAT_FASTA,
FORMAT_FASTQ,
FORMAT_FASTA_GZ,
FORMAT_FASTQ_GZ,
FORMAT_FASTA_BZ2,
FORMAT_FASTQ_BZ2,
FORMAT_EXPORT
};

/**
 *
 * This is a factory that produces products.
 * The product is a singleton at the moment.
 *
 * \author Sébastien Boisvert
 */
class LoaderFactory{

	int m_type;
	SffLoader m_sff;
	ColorSpaceLoader m_color;
	FastqLoader m_fastq;	
	FastaLoader m_fasta;	
	FastaLoaderForReads m_fastaLoader;
	ExportLoader m_export;

	#ifdef CONFIG_HAVE_LIBZ
	FastqGzLoader m_fastqgz;
	FastaGzLoader m_fastagz;
	#endif

	#ifdef CONFIG_HAVE_LIBBZ2
	FastqBz2Loader m_fastqbz2;
	FastaBz2Loader m_fastabz2;
	#endif

	bool hasSuffix(const char*fileName,const char*suffix);
public:
	LoaderInterface*makeLoader(string file);
};

#endif
