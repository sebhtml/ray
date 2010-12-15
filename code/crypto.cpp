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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<crypto.h>


/*
 * Basically, we take a 64-bit unsigned integer (the sign does not matter..)
 * and we compute the image corresponding to a uniform distribution.
 *
 * This uses some magic, of course!
 */
// see http://www.concentric.net/~Ttwang/tech/inthash.htm 64 bit Mix Functions
uint64_t uniform_hashing_function_1_64_64(uint64_t key){
	// some magic here and there.
	key = (~key) + (key << 21); 
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); 
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); 
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

uint64_t uniform_hashing_function_2_64_64(uint64_t key){
	key = (~key) + (key << 18); 
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	return key;
}

