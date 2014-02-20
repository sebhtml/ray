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

#include "Logger.h"


Logger * Logger::m_singleton = NULL;

Logger::Logger() {

	m_activeStream = &cout;

	m_mockup.open("/dev/null");

	m_inactiveStream = &m_mockup;

	m_levels.insert(INFO);
	m_levels.insert(WARNING);
	m_levels.insert(ERROR);
	m_levels.insert(FATAL);
}

Logger::~Logger() {

	m_mockup.close();

	m_activeStream = NULL;

	m_inactiveStream = NULL;
}


Logger * Logger::getSingleton() {

	if(m_singleton == NULL) {
		m_singleton = new Logger();
	}

	return m_singleton;
}

ostream & Logger::log(int level) {

	if(m_levels.count(level) > 0) {

		ostream & stream = *m_activeStream;

		stream << "[Logger] ";
		stream << toString(level) << " ";
		stream << "(" << level << ")" << " ";
		return stream;
	} else {
		return *m_inactiveStream;
	}
}

string Logger::toString(int level) {

	if(level == INFO)
		return "INFO";
	if(level == WARNING)
		return "WARNING";

	if(level == ERROR)
		return "ERROR";

	if(level == FATAL)
		return "FATAL";

	return "OTHER";
}
