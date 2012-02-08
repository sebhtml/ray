/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _unitTest
#define _unitTest

/* assertEquals is a macro because we want line numbers */

#define assertEquals(expected,actual) \
if(expected!=actual) {\
	cout<<endl; \
	cout<<"FAIL"<<endl; \
	cout<<"Failed unit test (assertEquals)"<<endl; \
	cout<<"Expected: "<<expected<<endl; \
	cout<<"Actual: "<<actual<<endl; \
	cout<<"File: "<<__FILE__<<endl; \
	cout<<"Function: "<<__func__<<endl; \
	cout<<"Line: "<<__LINE__<<endl; \
}else{ \
	cout<<endl; \
	cout<<"PASS"<<endl; \
}

#define assertIsLower(expected,actual) \
if(expected<=actual) {\
	cout<<endl; \
	cout<<"FAIL"<<endl; \
	cout<<"Failed unit test (assertIsLower)"<<endl; \
	cout<<"Expected: "<<expected<<endl; \
	cout<<"Actual: "<<actual<<endl; \
	cout<<"File: "<<__FILE__<<endl; \
	cout<<"Function: "<<__func__<<endl; \
	cout<<"Line: "<<__LINE__<<endl; \
}else{ \
	cout<<endl; \
	cout<<"PASS"<<endl; \
}

#define assertIsGreater(expected,actual) \
if(expected>=actual) {\
	cout<<endl; \
	cout<<"FAIL"<<endl; \
	cout<<"Failed unit test (assertIsGreater)"<<endl; \
	cout<<"Expected: "<<expected<<endl; \
	cout<<"Actual: "<<actual<<endl; \
	cout<<"File: "<<__FILE__<<endl; \
	cout<<"Function: "<<__func__<<endl; \
	cout<<"Line: "<<__LINE__<<endl; \
}else{ \
	cout<<endl; \
	cout<<"PASS"<<endl; \
}

#endif
