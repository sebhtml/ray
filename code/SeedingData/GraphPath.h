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

#include <code/Mock/constants.h>
#include <code/KmerAcademyBuilder/Kmer.h>

#include <RayPlatform/store/CarriageableItem.h>

#include <vector>
using namespace std;

#ifdef CONFIG_PATH_STORAGE_BLOCK

/*
 * The number of symbols per block.
 */
#define CONFIG_PATH_BLOCK_SIZE 4096

/*
 * Each block has 128 uint64_t objects, which can store 4096 nucleotides.
 * 4096*2 = 8192 bits
 * 128 * 8 * 8 = 8192 bits
 */

#define NUMBER_OF_64_BIT_INTEGERS ( CONFIG_PATH_BLOCK_SIZE * BITS_PER_NUCLEOTIDE / sizeof(uint64_t) / BITS_PER_BYTE )

class GraphPathBlock {

public:
	uint64_t m_content[NUMBER_OF_64_BIT_INTEGERS];
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
 * the PathHandle should be here directly instead of being a separate instance.
 * \date 2013-07-31 PathHandle is now a class on its own.
 *
 * \author Sébastien Boisvert
 */
class GraphPath  : public CarriageableItem {

	bool m_deleted;

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

	int getBlockSize()const;

	char readSymbolInBlock(int position)const;
	void writeSymbolInBlock(int position,char symbol);

	void addBlock();

public:
	GraphPath();

	int size()const;
	void at(int i,Kmer*value)const;

	CoverageDepth getCoverageAt(int i)const;

	void push_back(const Kmer*a);
	void getVertices(vector<Kmer>*value)const;
	void clear();

	void addCoverageValue(CoverageDepth value);
	void setCoverageValueAt(int position, CoverageDepth value);
	void reserveSpaceForCoverage();

	CoverageDepth getPeakCoverage()const;
	void resetCoverageValues();

	void computePeakCoverage();
	void reserve(int size);

	void setKmerLength(int kmerLength);

	bool isDeleted();
	void markAsDeleted();

	int load(const char * buffer);
	int dump(char * buffer) const;
	int getRequiredNumberOfBytes() const;

	int getKmerLength() const;

	/**
	 * Put the reversed content
	 * in another object.
	 */
	void reverseContent(GraphPath & newPath) const;

	void appendPath(const GraphPath & path);
};

bool comparePaths(const GraphPath & a,const GraphPath & b);

#endif
