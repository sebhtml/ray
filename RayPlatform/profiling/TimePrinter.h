/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _TimePrinter
#define _TimePrinter


// TODO: move this class in the core
#include<time.h>
#include<vector>
#include <fstream>
#include<string>
using namespace std;

/**
 * Prints the current local time.
 * Also, prints the elapsed time since the beginning.
 * \author Sébastien Boisvert
 */
class TimePrinter{
	ofstream m_file;
	bool m_fileSet;
	time_t m_last;
	time_t m_startingTime;
	time_t m_lastTime;
	time_t m_endingTime;
	vector<string> m_descriptions;
	vector<int> m_durations;

	void printDifference(int s,ostream*stream);

	void printDurationsInStream(ostream*stream,struct tm*t);
	void printElapsedTimeInStream(ostream*stream, string description,struct tm*timeinfo,
int differenceWithLast);
public:
	void printElapsedTime(string description);
	void setFile(string prefix);
	void constructor();
	void printDurations();
	void printDifferenceFromStart(int rank);
};

#endif
