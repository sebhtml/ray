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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _TimePrinter
#define _TimePrinter

#include<time.h>
#include<vector>
#include<string>
using namespace std;

class TimePrinter{
	time_t m_last;
	time_t m_startingTime;
	time_t m_lastTime;
	time_t m_endingTime;
	vector<string> m_descriptions;
	vector<int> m_durations;

	void printDifference(int s);
public:
	void printElapsedTime(string description);
	TimePrinter();
	void printDurations();
	void printDifferenceFromStart(int rank);
};

#endif
