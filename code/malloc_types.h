/*

Ray
Copyright (C)  2010  Sébastien Boisvert

http://DeNovoAssembler.SourceForge.Net/

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You have received a copy of the GNU General Public License
along with this program (COPYING).  
see <http://www.gnu.org/licenses/>

*/

#ifndef __MALLOC_TYPES
#define __MALLOC_TYPES

extern const char* MALLOC_TYPES[];

enum{
RAY_MALLOC_TYPE_SEED_EXTENDER_CACHE,
RAY_MALLOC_TYPE_EXTENSION_DATA_TREES,
RAY_MALLOC_TYPE_EXTENSION_DATA_ALLOCATOR,
RAY_MALLOC_TYPE_LIBRARY_ALLOCATOR,
RAY_MALLOC_TYPE_AMOS,
RAY_MALLOC_TYPE_GRID_TABLE_DATA,
RAY_MALLOC_TYPE_GRID_TABLE_SIZES,
RAY_MALLOC_TYPE_DUMMY
};

#endif
