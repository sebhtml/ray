/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _AssemblySeed_h
#define _AssemblySeed_h

#include <vector>
#include <plugin_KmerAcademyBuilder/Kmer.h>
using namespace std;

/**
 * This class describes objects representing assembly seeds.
 * An assembly seed is a path in the de Bruijn graph.
 * It is likely unique in the target genome (hopefully !)
 * \author pro grammer: Sébastien Boisvert (good with grammar)
 * \author co-designer: The Ray committee of wise people (E. God.)
 * \date updated 2012-06-21 for the release of v2.0.0
 */
class AssemblySeed{
	vector<Kmer> m_vertices;
	vector<CoverageDepth> m_coverageValues;

	CoverageDepth m_peakCoverage;

/** computes locality with a weighted mean (locality object is peak coverage) **/
	void computePeakCoverageUsingMean();

/** computes locality with a mode (locality object is peak coverage) **/
	void computePeakCoverageUsingMode();

public:
	AssemblySeed();

	int size()const;
	Kmer*at(int i);
	CoverageDepth getCoverageAt(int i);
	
	void push_back(Kmer*a);
	vector<Kmer>*getVertices();
	void clear();

	void addCoverageValue(CoverageDepth value);
	CoverageDepth getPeakCoverage();
	void resetCoverageValues();

	void computePeakCoverage();
};

#endif
