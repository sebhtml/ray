/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <cryptography/crypto.h>
#include <string.h>

/*
 * Basically, we take a 64-bit unsigned integer (the sign does not matter..)
 * and we compute the image corresponding to a uniform distribution.
 *
 * see http://www.concentric.net/~Ttwang/tech/inthash.htm 64 bit Mix Functions
 */
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

/*
 * based on uniform_hashing_function_1_64_64, but with different values
 */
uint64_t uniform_hashing_function_2_64_64(uint64_t key){
	key = (~key) + (key << 31); 
	key = key ^ (key >> 14);
	key = (key + (key << 7)) + (key << 11); 
	key = key ^ (key >> 13);
	key = (key + (key << 4)) + (key << 8); 
	key = key ^ (key >> 44);
	key = key + (key << 6);
	return key;
}


