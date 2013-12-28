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


#ifndef PathHandleHeader
#define PathHandleHeader

#include <RayPlatform/store/CarriageableItem.h>

#include <ostream>
using namespace std;

#include <stdint.h>

/** 
 * the identifier for a path in the de Bruijn graph 
 */
class PathHandle : public CarriageableItem {

	uint64_t m_value;

public:
	PathHandle(const uint64_t & value);
	PathHandle();
	void operator=(const PathHandle &b);
	bool operator<(const PathHandle & b)const;
	bool operator>(const PathHandle & b)const;
	bool operator<=(const PathHandle & b)const;
	bool operator>=(const PathHandle & b)const;
	bool operator!=(const PathHandle & b)const;
	bool operator==(const PathHandle & b)const;
	void operator=(const uint64_t & value);
	const uint64_t & operator () () const;
	uint64_t & operator () ();
	uint64_t operator / (uint64_t value);
	uint64_t operator - (uint64_t value);
	uint64_t operator * (uint64_t value);
	uint64_t operator + (uint64_t value);
	uint64_t operator % (uint64_t value);

	// add these 2 as friends.
	friend ostream & operator <<(ostream & stream, const PathHandle & handle);
	friend ostream & operator >>(ostream & stream, const PathHandle & handle);

	const uint64_t & getValue() const;
	uint64_t & getValue();

	int load(const char * buffer);
	int dump(char * buffer) const;
	int getRequiredNumberOfBytes() const;
};

/**
 * prototype for operators.
 */
ostream & operator <<(ostream & stream, const PathHandle & handle);
ostream & operator >>(ostream & stream, const PathHandle & handle);

#endif
