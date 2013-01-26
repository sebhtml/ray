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

#ifndef _GraphPath_h
#define _GraphPath_h

#include <code/plugin_Mock/constants.h>
#include <code/plugin_KmerAcademyBuilder/Kmer.h>

#include <vector>
using namespace std;

#ifdef CONFIG_PATH_STORAGE_BLOCK

#define CONFIG_PATH_BLOCK_SIZE 4096

class GraphPathBlock{
public:
	char m_content[CONFIG_PATH_BLOCK_SIZE];
};
#endif

/**
 * This class describes objects representing assembly seeds.
 * An assembly seed is a path in the de Bruijn graph.
 * It is likely unique in the target genome (hopefully !)
 *
 * \author pro-grammer: Sébastien Boisvert (good with grammar)
 * \author co-designer: The Ray committee of wise people (E. God.)
 * \date updated 2012-06-21 for the release of v2.0.0
 *
 * TODO: the PathHandle should be here directly instead of being a separate instance.
 */
class GraphPath{

	bool m_errorRaised;
	int m_kmerLength;

#ifdef CONFIG_PATH_STORAGE_DEFAULT
	vector<Kmer> m_vertices;
#elif defined(CONFIG_PATH_STORAGE_BLOCK)
	vector<GraphPathBlock> m_blocks;
	int m_size;
#endif

	vector<CoverageDepth> m_coverageValues;

	CoverageDepth m_peakCoverage;

/** computes locality with a weighted mean (locality object is peak coverage) **/
	void computePeakCoverageUsingMean();

/** computes locality with a mode (locality object is peak coverage) **/
	void computePeakCoverageUsingMode();

	void computePeakCoverageUsingStaggeredMean();

	bool m_hasPeakCoverage;

#ifdef CONFIG_PATH_STORAGE_BLOCK
	void readObjectInBlock(int position,Kmer*object)const;
	void writeObjectInBlock(const Kmer*a);
#endif

	bool canBeAdded(const Kmer*value)const;

public:
	GraphPath();

	int size()const;
	void at(int i,Kmer*value)const;

	CoverageDepth getCoverageAt(int i)const;
	
	void push_back(const Kmer*a);
	void getVertices(vector<Kmer>*value)const;
	void clear();

	void addCoverageValue(CoverageDepth value);
	CoverageDepth getPeakCoverage()const;
	void resetCoverageValues();

	void computePeakCoverage();
	void reserve(int size);

	void setKmerLength(int kmerLength);
};

#endif
