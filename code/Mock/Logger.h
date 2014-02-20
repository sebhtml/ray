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

#ifndef _Logger
#define _Logger

#include <iostream>
#include <fstream>
#include <set>
#include <string>
using namespace std;

class Logger;

/**
 *
 * Fancy logger class with same interface
 * as Google Glog (LOG(INFO) << bla bla bla
 *
 * \see http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * \see http://easylogging.org/
 *
 * \author Sébastien Boisvert
 */
class Logger {

private:

	static Logger * m_singleton;

	set<int> m_levels;

	ofstream m_mockup;
	ostream * m_activeStream;
	ostream * m_inactiveStream;

	Logger();

	string toString(int level);
public:


	enum {

		INFO = 99,
		WARNING,
		ERROR,
		FATAL,

		// other that are specific to Ray

		MERGER_DEBUG,
		NETWORK_TEST_DEBUG
	};


	~Logger();
	static Logger * getSingleton();

	ostream & log(int level);

};


#define LOG(LEVEL)  ( Logger::getSingleton()->log(Logger::LEVEL) )

#endif
