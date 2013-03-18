/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#ifndef _AnnihilationWorker_h
#define _AnnihilationWorker_h

#include <code/plugin_SeedingData/GraphPath.h>

#include <RayPlatform/scheduling/Worker.h>

#include <stdint.h>

/**
 * This is a worker that analyze a seed.
 *
 * \author Sébastien Boisvert
 */
class AnnihilationWorker: public Worker{

	uint64_t m_identifier;
	bool m_done;
	GraphPath * m_seed;

public:
	void work();

	bool isDone();

	WorkerHandle getWorkerIdentifier();

	void initialize(uint64_t identifier, GraphPath*seed);
};

#endif
