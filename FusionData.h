/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#ifndef _FusionData
#define _FusionData

#include<Direction.h>
#include<vector>
#include<set>
#include<map>
using namespace std;

class FusionData{
public:
	// FUSION
	bool m_fusionStarted;
	bool m_FUSION_direct_fusionDone;
	bool m_FUSION_first_done;
	int m_FUSION_numberOfRanksDone;
	bool m_FUSION_last_done;
	int m_FUSION_path_id;
	int m_FUSION_numberOfPaths;
	bool m_FUSION_paths_requested;
	bool m_FUSION_paths_received;
	vector<Direction> m_FUSION_firstPaths;
	bool m_FUSION_path_received;
	map<int,vector<Direction> > m_FUSION_cachedDirections;
	int m_FUSION_receivedLength;
	bool m_FUSION_reverse_fusionDone;
	vector<Direction> m_FUSION_lastPaths;
	vector<int> m_FUSION_matches;
	bool m_FUSION_matches_done;
	bool m_FUSION_pathLengthReceived;
	bool m_FUSION_matches_length_done;
	int m_FUSION_match_index;
	bool m_FUSION_pathLengthRequested;
	vector<Direction> m_FUSION_receivedPaths;
	bool m_FUSION_path_requested;
	Direction m_FUSION_receivedPath;
	map<int,int> m_FUSION_identifier_map;
	set<int> m_FUSION_eliminated;
};

#endif
