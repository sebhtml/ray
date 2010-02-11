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


#ifndef _Node
#define _Node

#include<types.h>
#include<iostream>
#include<stdlib.h>
using namespace std;

template<class AVL_KEY,class AVL_VALUE>
class Node{

public:
	Node*m_left;
	Node*m_right;

	AVL_KEY m_key;
	AVL_VALUE m_value;
	char m_balance;
	char m_height;
	void update();
	void stat();
	Node(AVL_KEY key);
	AVL_VALUE getValue();
	void setValue(AVL_VALUE value);
	AVL_KEY getKey();
	Node*getLeft();
	Node*getRight();
	void setLeft(Node*node);
	void setRight(Node*node);
	char getBalance();
	char getHeight();
};

template<class AVL_KEY,class AVL_VALUE>
void Node<AVL_KEY,AVL_VALUE>::setValue(AVL_VALUE value){
	m_value=value;
}

template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>::Node(AVL_KEY key){
	m_key=key;
	m_left=NULL;
	m_right=NULL;
	m_balance=0;
	m_height=0;
}

template<class AVL_KEY,class AVL_VALUE>
void Node<AVL_KEY,AVL_VALUE>::stat(){
	cout<<"Key: "<<getKey()<<endl;
	cout<<"Balance: "<<(int)getBalance()<<endl;
	cout<<"Height: "<<(int)m_height<<endl;
}


template<class AVL_KEY,class AVL_VALUE>
char Node<AVL_KEY,AVL_VALUE>::getBalance(){
	return m_balance;
}

template<class AVL_KEY,class AVL_VALUE>
char Node<AVL_KEY,AVL_VALUE>::getHeight(){
	return m_height;
}

template<class AVL_KEY,class AVL_VALUE>
void Node<AVL_KEY,AVL_VALUE>::update(){
	int leftHeight=0;
	int rightHeight=0;
	if(m_left!=NULL){
		leftHeight=m_left->m_height;
	}
	if(m_right!=NULL){
		rightHeight=m_right->m_height;
	}
	m_height=leftHeight;
	if(rightHeight>m_height){
		m_height=rightHeight;
	}
	m_height++;
	m_balance=rightHeight-leftHeight;
}


template<class AVL_KEY,class AVL_VALUE>
AVL_VALUE Node<AVL_KEY,AVL_VALUE>::getValue(){
	return m_value;
}

template<class AVL_KEY,class AVL_VALUE>
AVL_KEY Node<AVL_KEY,AVL_VALUE>::getKey(){
	return m_key;
}



template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>*Node<AVL_KEY,AVL_VALUE>::getLeft(){
	return m_left;
}

template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>*Node<AVL_KEY,AVL_VALUE>::getRight(){
	return m_right;
}

template<class AVL_KEY,class AVL_VALUE>
void Node<AVL_KEY,AVL_VALUE>::setLeft(Node<AVL_KEY,AVL_VALUE>*node){
	m_left=node;
}

template<class AVL_KEY,class AVL_VALUE>
void Node<AVL_KEY,AVL_VALUE>::setRight(Node<AVL_KEY,AVL_VALUE>*node){
	m_right=node;
}




#endif
