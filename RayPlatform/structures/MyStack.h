/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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

#ifndef _MyStack
#define _MyStack

#include<assert.h>
#define _MAX_STACK_SIZE 1024

/**
 * \author Sébastien Boisvert
 */
template<class TYPE>
class MyStack{
	TYPE m_array[_MAX_STACK_SIZE]; // maximum size 
	int m_size;
public:
	TYPE top()const;
	void pop();
	int size()const;
	void push(TYPE a);
	MyStack();
	bool empty()const;
};


template<class TYPE>
bool MyStack<TYPE>::empty()const{
	return m_size==0;
}

template<class TYPE>
MyStack<TYPE>::MyStack(){
	m_size=0;
}

template<class TYPE>
TYPE MyStack<TYPE>::top()const{
	return m_array[m_size-1];
}

template<class TYPE>
void MyStack<TYPE>::pop(){
	m_size--;
}

template<class TYPE>
int MyStack<TYPE>::size() const{
	return m_size;
}

template<class TYPE>
void MyStack<TYPE>::push(TYPE a){
	#ifdef ASSERT
	assert(m_size!=_MAX_STACK_SIZE);
	#endif
	m_array[m_size++]=a;
}

#endif

