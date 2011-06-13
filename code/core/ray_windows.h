/*
Ray
Copyright (C)  2011  Sébastien Boisvert

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

#ifndef _ray_windows
#define _ray_windows

/*
 * Include those libraries for Microsoft Visual C++
 */
#ifdef _MSC_VER
#include <xiosbase>
#include <stdexcept>
/* http://msdn.microsoft.com/en-us/library/b0084kay%28VS.80%29.aspx */
#define __func__ __FUNCTION__ 
#endif

#endif
