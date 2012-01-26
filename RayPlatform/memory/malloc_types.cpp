/*

Ray
Copyright (C)  2010, 2011  Sébastien Boisvert

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

#include <memory/malloc_types.h>

/* use a macro to define the array 
 * #x means x in quotes   FOO becomes "FOO" */

#define ITEM(x) #x,

const char* MALLOC_TYPES[]={
#include <malloc_types.txt>
};

#undef ITEM
