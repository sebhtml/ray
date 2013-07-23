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


#ifndef ReadHandleHeader
#define ReadHandleHeader

#include <stdint.h>

#include <ostream>
using namespace std;

class ReadHandle {

	uint64_t m_value;

public:
	ReadHandle(const uint64_t & value);
	ReadHandle();
	void operator=(const ReadHandle &b);
	bool operator<(const ReadHandle & b)const;
	bool operator>(const ReadHandle & b)const;
	bool operator<=(const ReadHandle & b)const;
	bool operator>=(const ReadHandle & b)const;
	bool operator!=(const ReadHandle & b)const;
	bool operator==(const ReadHandle & b)const;
	void operator=(const uint64_t & value);
	const uint64_t & operator () () const;
	uint64_t & operator () ();
	uint64_t operator / (uint64_t value);
	uint64_t operator - (uint64_t value);
	uint64_t operator * (uint64_t value);
	uint64_t operator + (uint64_t value);

	friend ostream & operator <<(ostream & stream, const ReadHandle & handle);

	const uint64_t & getValue() const;
	uint64_t & getValue();
};

#endif
