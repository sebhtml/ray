/*
 * Copyright 2014 Sébastien Boisvert
 *
 * This file is part of Ray.
 *
 * Ray Surveyor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Ray Surveyor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
 */



// License: GPLv3
//

#ifndef _PathMaster
#define _PathMaster


#include <code/Mock/Parameters.h>
#include <code/SeedingData/GraphPath.h>

/**
 * A master of paths that can compare and combine paths.
 *
 * When working with a reverse complement DNA object,
 * position 99 on reverse strand is equivalent to LENGTH - 1 - 99 on FORWARD
 *
 * \author Sébastien Boisvert
 *
 */
class PathMaster {

	Parameters * m_parameters;
public:

	PathMaster();
	~PathMaster();


	void initialize(Parameters * parameters);

	void compare(GraphPath & path1, bool strand1, GraphPath & path2, bool strand2,
		int &bestMatches, int &bestLast1, int &bestLast2);



	void combine(GraphPath & newPath, GraphPath & path1, bool strand1,
							GraphPath & path2, bool strand2, int bestLast1, int bestLast2);


	void display(GraphPath & path, bool strand, int start, int end, int special);
};

#endif
