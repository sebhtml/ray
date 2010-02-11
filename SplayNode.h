/*
 	Ray
    Copyright (C) 2009, 2010  Sébastien Boisvert

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


#ifndef _SplayNode
#define _SplayNode

#include<iostream>
using namespace std;

template<class KEY,class VALUE>
class SplayNode{
public:

	KEY m_key;
	VALUE m_value;
	SplayNode<KEY,VALUE>*m_left;
	SplayNode<KEY,VALUE>*m_right;
	SplayNode(KEY key);
	SplayNode();
	void setLeft(SplayNode<KEY,VALUE>*vertex);
	void setRight(SplayNode<KEY,VALUE>*vertex);
	void setParent(SplayNode<KEY,VALUE>*vertex);
	KEY getKey();
	void init(KEY key);
	SplayNode<KEY,VALUE>*getLeft();
	SplayNode<KEY,VALUE>*getRight();
	SplayNode<KEY,VALUE>*getParent();
	VALUE*getValue();
	void setValue(VALUE v);
};

template<class KEY,class VALUE>
VALUE*SplayNode<KEY,VALUE>::getValue(){
	return &m_value;
}

template<class KEY,class VALUE>
void SplayNode<KEY,VALUE>::setValue(VALUE v){
	m_value=v;
}

template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayNode<KEY,VALUE>::getRight(){
	return m_right;
}

template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayNode<KEY,VALUE>::getLeft(){
	return m_left;
}


template<class KEY,class VALUE>
void SplayNode<KEY,VALUE>::init(KEY key){
	m_key=key;
	m_left=NULL;
	m_right=NULL;
}

template<class KEY,class VALUE>
KEY SplayNode<KEY,VALUE>::getKey(){
	return m_key;
}

template<class KEY,class VALUE>
void SplayNode<KEY,VALUE>::setLeft(SplayNode<KEY,VALUE>*vertex){
	m_left=vertex;
	if(vertex!=NULL)
		vertex->setParent(this);
}


template<class KEY,class VALUE>
void SplayNode<KEY,VALUE>::setRight(SplayNode<KEY,VALUE>*vertex){
	m_right=vertex;
	if(vertex!=NULL)
		vertex->setParent(this);
}

template<class KEY,class VALUE>
SplayNode<KEY,VALUE>::SplayNode(KEY key){
	init(key);
}


template<class KEY,class VALUE>
SplayNode<KEY,VALUE>::SplayNode(){
	m_left=NULL;
	m_right=NULL;
}

#endif

