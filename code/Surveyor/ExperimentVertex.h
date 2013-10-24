/*
    Copyright 2013 Sébastien Boisvert
    Copyright 2013 Université Laval
    Copyright 2013 Centre Hospitalier Universitaire de Québec

    This file is part of Ray Surveyor.

    Ray Surveyor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    Ray Surveyor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ExperimentVertexHeader
#define ExperimentVertexHeader

#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/Searcher/VirtualKmerColor.h>
#include <code/Searcher/ColorSet.h>

/**
 * This class stores a vertex for the multi-sample
 * analysis problem.
 *
 * \author Sébastien Boisvert
 */
class ExperimentVertex {


private:

	VirtualKmerColorHandle m_color;

	// TODO: don't store kmers here since it is the key elsewhere already !
	// Otherwise, this consumes more memory and this is not good.
	Kmer m_kmer;

public:

	ExperimentVertex();
	~ExperimentVertex();

	Kmer getKey() const;
	void setKey(Kmer & kmer);

	void setVirtualColor(VirtualKmerColorHandle handle);
	VirtualKmerColorHandle getVirtualColor() const;
};

#endif
