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

#ifndef SequenceFileDetectorHeader
#define SequenceFileDetectorHeader

#include <RayPlatform/core/OperatingSystem.h>

/**
 *
 * This class receives a directory
 * and returns the valid genomic files.
 *
 * \author Sébastien Boisvert
 */
class SequenceFileDetector {

	char m_directorySeparator;

	vector<string> m_leftFiles;
	vector<string> m_rightFiles;
	vector<string> m_singleFiles;

	void gatherAllFiles(string & root, vector<string> & rawFiles);

public:

	SequenceFileDetector();

	/**
	 * Detect all supported files in a directory, recursively.
	 */
	void detectSequenceFiles(string & directory);
	vector<string> & getLeftFiles();
	vector<string> & getRightFiles();
	vector<string> & getSingleFiles();
};

#endif
