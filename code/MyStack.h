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


#ifndef _MyStack
#define _MyStack

template<class TYPE>
class MyStack{
	TYPE m_array[1024]; // maximum size 
	int m_size;
public:
	TYPE top();
	void pop();
	int size();
	void push(TYPE a);
	MyStack();
	bool empty();
};


template<class TYPE>
bool MyStack<TYPE>::empty(){
	return m_size==0;
}

template<class TYPE>
MyStack<TYPE>::MyStack(){
	m_size=0;
}

template<class TYPE>
TYPE MyStack<TYPE>::top(){
	return m_array[m_size-1];
}

template<class TYPE>
void MyStack<TYPE>::pop(){
	m_size--;
}

template<class TYPE>
int MyStack<TYPE>::size(){
	return m_size;
}

template<class TYPE>
void MyStack<TYPE>::push(TYPE a){
	m_array[m_size++]=a;
}

#endif

