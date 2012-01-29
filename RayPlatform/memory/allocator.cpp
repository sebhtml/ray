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

#include <memory/allocator.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <core/OperatingSystem.h>
//#include <core/constants.h>
using namespace std;

/**
 * wrapper around malloc
 */
void*__Malloc(int c,const char*description,bool show){
	#ifdef ASSERT
	if(c==0){
		cout<<"Requested "<<c<<" bytes of type "<<description<<endl;
	}
	assert(c!=0);
	assert(c>0);
	#endif
	void*a=NULL;
	a=malloc(c);
	if(a==NULL){
		cout<<"Critical exception: The system is out of memory, returned NULL."<<endl;
		cout<<"Requested "<<c<<" bytes of type "<<description<<endl;
		exit(EXIT_NO_MORE_MEMORY);
	}

	assert(a!=NULL);

	if(show){
		printf("%s %i\t%s\t%i bytes, ret\t%p\t%s\n",__FILE__,__LINE__,__func__,c,a,description);
		fflush(stdout);
	}
	return a;
}

/**
 * wrapper around free
 */
void __Free(void*a,const char*description,bool show){
	if(show){
		printf("%s %i\t%s\t%p\t%s\n",__FILE__,__LINE__,__func__,a,description);
		fflush(stdout);
	}

	free(a);
}


