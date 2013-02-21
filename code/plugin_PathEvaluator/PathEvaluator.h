/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2013 Sébastien Boisvert

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

#ifndef _PathEvaluator_h
#define _PathEvaluator_h

#include <RayPlatform/core/ComputeCore.h>

__DeclarePlugin(PathEvaluator);

__DeclareMasterModeAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS);

/**
 * Evaluate the quality of paths.
 *
 * \author Sébastien Boisvert
 **/
class PathEvaluator :  public CorePlugin {

	__AddAdapter(PathEvaluator,RAY_MASTER_MODE_EVALUATE_PATHS);

	MasterMode RAY_MASTER_MODE_EVALUATE_PATHS;
	MasterMode RAY_MASTER_MODE_ASK_EXTENSIONS;
public:

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

	void call_RAY_MASTER_MODE_EVALUATE_PATHS();
};

#endif /* _PathEvaluator_h */
