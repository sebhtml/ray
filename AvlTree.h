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


#ifndef _AvlTree
#define _AvlTree

#include<types.h>
#include<stack>
#include<Node.h>
#include<AvlTree.h>
#include<assert.h>
#include<stdlib.h>
#include<iostream>
using namespace std;

template<class AVL_KEY,class AVL_VALUE>
class AvlTree{
	VertexIndex m_size;
	bool m_inserted;
	Node<AVL_KEY,AVL_VALUE>*m_root;
	void insertFrom(Node<AVL_KEY,AVL_VALUE>*&node,AVL_KEY key);
	void showFrom(Node<AVL_KEY,AVL_VALUE>*node,int i);
	void balance(Node<AVL_KEY,AVL_VALUE>*&parent);
public:
	AvlTree();
	Node<AVL_KEY,AVL_VALUE>*getRoot();
	~AvlTree();
	void clear();
	VertexIndex size();
	Node<AVL_KEY,AVL_VALUE>*insert(AVL_KEY key);
	int count(AVL_KEY key);
	Node<AVL_KEY,AVL_VALUE>*find(AVL_KEY key);
	void stat();
	void show();
	bool isBalanced();
	bool inserted();
};


template<class AVL_KEY,class AVL_VALUE>
AvlTree<AVL_KEY,AVL_VALUE>::AvlTree(){
	m_root=NULL;
	m_size=0;
	m_inserted=false;
}

template<class AVL_KEY,class AVL_VALUE>
AvlTree<AVL_KEY,AVL_VALUE>::~AvlTree(){
	clear();
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::clear(){
	stack<Node<AVL_KEY,AVL_VALUE>*> aStack;
	if(m_root!=NULL){
		aStack.push(m_root);
	}
	while(aStack.size()>0){
		Node<AVL_KEY,AVL_VALUE>*aNode=aStack.top();
		aStack.pop();
		if(aNode->m_left!=NULL){
			aStack.push(aNode->m_left);
		}
		if(aNode->m_right!=NULL){
			aStack.push(aNode->m_right);
		}
		delete aNode;
	}
	m_root=NULL;
	m_size=0;
}

template<class AVL_KEY,class AVL_VALUE>
VertexIndex AvlTree<AVL_KEY,AVL_VALUE>::size(){
	return m_size;
}

template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>*AvlTree<AVL_KEY,AVL_VALUE>::insert(AVL_KEY key){
	Node<AVL_KEY,AVL_VALUE>*node=find(key);
	if(node!=NULL){
		m_inserted=false;
		return node;
	}
	m_size++;
	if(m_root==NULL){
		m_root=new Node<AVL_KEY,AVL_VALUE>(key);
	}else{
		insertFrom(m_root,key);
		m_root->update();
		balance(m_root);
	}
	m_inserted=true;
	return find(key);
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::insertFrom(Node<AVL_KEY,AVL_VALUE>*&nodePtr,AVL_KEY key){
	if(key<(nodePtr)->m_key){
		if((nodePtr)->m_left==NULL){
			(nodePtr)->m_left=(new Node<AVL_KEY,AVL_VALUE>(key));
		}else{
			insertFrom((nodePtr)->m_left,key);
			(nodePtr)->m_left->update();
			balance(nodePtr->m_left);
		}
	}else{
		if((nodePtr)->m_right==NULL){
			(nodePtr)->m_right=new Node<AVL_KEY,AVL_VALUE>(key);
		}else{
			insertFrom((nodePtr)->m_right,key);
			(nodePtr)->m_right->update();
			balance(nodePtr->m_right);
		}
	}
}


template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>*AvlTree<AVL_KEY,AVL_VALUE>::find(AVL_KEY key){
	Node<AVL_KEY,AVL_VALUE>*current=m_root;
	while(current!=NULL){
		if(key<current->m_key){
			current=current->m_left;
		}else if(current->m_key<key){
			current=current->m_right;
		}else{
			#ifdef DEBUG_ASSEMBLER
			assert(current->m_key==key);
			#endif
			return current;
		}
	}
	#ifdef DEBUG_ASSEMBLER
	assert(current==NULL);
	#endif
	return current;
}

template<class AVL_KEY,class AVL_VALUE>
int AvlTree<AVL_KEY,AVL_VALUE>::count(AVL_KEY key){
	return find(key)!=NULL;
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::stat(){
	stack<Node<AVL_KEY,AVL_VALUE> > aStack;
	if(m_root!=NULL){
		aStack.push(m_root);
	}
	while(aStack.size()>0){
		Node<AVL_KEY,AVL_VALUE>*node=aStack.top();
		aStack.pop();
		if(node->m_left!=NULL){
			aStack.push(node->m_left);
		}
		if(node->m_right!=NULL){
			aStack.push(node->m_right);
		}
		node->stat();
	}
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::show(){
	showFrom(m_root,0);
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::showFrom(Node<AVL_KEY,AVL_VALUE>*node,int i){
	if(node==NULL)
		return;
	for(int j=0;j<i;j++){
		cout<<" ";
	}
	cout<<node->m_key<<endl;
	showFrom(node->m_left,i+1);
	showFrom(node->m_right,i+1);
}

template<class AVL_KEY,class AVL_VALUE>
bool AvlTree<AVL_KEY,AVL_VALUE>::isBalanced(){
	stack<Node<AVL_KEY,AVL_VALUE>*> aStack;
	if(m_root!=NULL){
		aStack.push(m_root);
	}
	while(aStack.size()>0){
		Node<AVL_KEY,AVL_VALUE>*node=aStack.top();
		aStack.pop();
		if(node->m_left!=NULL){
			aStack.push(node->m_left);
		}
		if(node->m_right!=NULL){
			aStack.push(node->m_right);
		}
		if(node->m_balance<-1||node->m_balance>1){
			cout<<(int)node->m_balance<<endl;
			return false;
		}
	}
	return true;
}

template<class AVL_KEY,class AVL_VALUE>
void AvlTree<AVL_KEY,AVL_VALUE>::balance(Node<AVL_KEY,AVL_VALUE>*&parent){
	if((parent)->m_balance>1){
		Node<AVL_KEY,AVL_VALUE>*t=parent;
		parent=(parent)->m_right;
		Node<AVL_KEY,AVL_VALUE>*t2=(parent)->m_left;
		(parent)->setLeft(t);
		t->setRight(t2);
		t->update();
		(parent)->update();
	}else if((parent)->m_balance<-1){
		Node<AVL_KEY,AVL_VALUE>*t=parent;
		parent=(parent)->m_left;
		Node<AVL_KEY,AVL_VALUE>*t2=(parent)->m_right;
		(parent)->setRight(t);
		t->setLeft(t2);
		t->update();
		(parent)->update();
	}	
}

template<class AVL_KEY,class AVL_VALUE>
Node<AVL_KEY,AVL_VALUE>*AvlTree<AVL_KEY,AVL_VALUE>::getRoot(){
	return m_root;
}

template<class AVL_KEY,class AVL_VALUE>
bool AvlTree<AVL_KEY,AVL_VALUE>::inserted(){
	return m_inserted;
}


#endif
