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

#ifndef _SplayTree
#define _SplayTree

#include<structures/MyStack.h>
#include<vector>
#include<stdlib.h>
#include<core/common_functions.h>
#include<structures/SplayNode.h>
#include<iostream>
#include<memory/MyAllocator.h>
using namespace std;

/**
 * the splay tree is a binary tree. If a key is asked often, the key will be near the root, otherwise, it will be deeper.
 * this eases the processing because sequencing errors are kept far away from the root.
 *
 * \see http://www.eli.sdsu.edu/courses/fall95/cs660/notes/splay/Splay.html
 * \see http://www.cs.nyu.edu/algvis/java/SplayTree.html
 * \see ftp://ftp.cs.cmu.edu/user/sleator/splaying/SplayTree.java
 * \author Sébastien Boisvert
*/
template<class KEY,class VALUE>
class SplayTree{
	SplayNode<KEY,VALUE>*m_root;
	uint64_t m_size;
	void splay(KEY key);
public:
	void constructor();
	// freeze the splay tree.
	bool remove(KEY key,bool reuse,MyAllocator*allocator);
	SplayNode<KEY,VALUE>*insert(KEY key,MyAllocator*allocator,bool*inserted);
	SplayNode<KEY,VALUE>*find(KEY key,bool frozen);
	SplayNode<KEY,VALUE>*findBinary(KEY key);
	void print();
	SplayNode<KEY,VALUE>*getRoot();
	uint64_t size();
	void clear();
};

template<class KEY,class VALUE>
void SplayTree<KEY,VALUE>::clear(){
	// the allocator will be released elsewhere
	m_root=NULL;
	m_size=0;
}

template<class KEY,class VALUE>
uint64_t SplayTree<KEY,VALUE>::size(){
	return m_size;
}

template<class KEY,class VALUE>
void SplayTree<KEY,VALUE>::constructor(){
	m_root=NULL;
	m_size=0;
}

/*
 *
 * based on http://www.cs.umbc.edu/courses/undergraduate/341/fall98/frey/ClassNotes/Class17/splay.html
 */
template<class KEY,class VALUE>
bool SplayTree<KEY,VALUE>::remove(KEY key,bool reuse,MyAllocator*allocator){
	// can't remove from an empty tree
	if(m_root==NULL){
		return false;
	}
	// make the node with key at root
	splay(key);

	// the key is not in the tree
	if(m_root->getKey()!=key){
		return false;
	}
	SplayNode<KEY,VALUE>*leftSubTree=m_root->getLeft();
	SplayNode<KEY,VALUE>*rightSubTree=m_root->getRight();
	SplayNode<KEY,VALUE>*toRemove=m_root;

	if(leftSubTree!=NULL){
		// we only work on the left subtree
		m_root=leftSubTree;

		// find the max node in left tree
		SplayNode<KEY,VALUE>*maxNodeInLeftTree=leftSubTree;
		while(maxNodeInLeftTree->getRight()!=NULL){
			maxNodeInLeftTree=maxNodeInLeftTree->getRight();
		}

		// make it the root of the subtree
		splay(maxNodeInLeftTree->getKey());

		// set the right subtree to the right tree of the root
		m_root->setRight(rightSubTree);
	}else{
		m_root=rightSubTree;
	}

	m_size--;

	// reuse the pointer 
	if(reuse){
		allocator->free(toRemove,sizeof(SplayNode<KEY,VALUE>));
	}
	return true;
}

template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayTree<KEY,VALUE>::getRoot(){
	return m_root;
}

/*
 * find x, if it is not there, call the last vertex visited during the search y, and splay y
 * then, add x as root, and put y as child of x (left or right)
 */
template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayTree<KEY,VALUE>::insert(KEY key,MyAllocator*allocator,bool*inserted){
	(*inserted)=false;
	if(m_root==NULL){
		m_root=(SplayNode<KEY,VALUE>*)allocator->allocate(sizeof(SplayNode<KEY,VALUE>));
		m_root->init(key);
		(*inserted)=true;
		m_size++;
		return m_root;
	}
	splay(key);
	if(m_root->getKey()==key)
		return m_root;
	SplayNode<KEY,VALUE>*n=(SplayNode<KEY,VALUE>*)allocator->allocate(sizeof(SplayNode<KEY,VALUE>));
	n->init(key);
	
	if(key<m_root->getKey()){
		n->m_left=m_root->m_left;
		n->m_right=m_root;
		m_root->m_left=NULL;
	}else{
		n->m_right=m_root->m_right;
		n->m_left=m_root;
		m_root->m_right=NULL;
	}
	m_root=n;
	m_size++;
	(*inserted)=true;
	return m_root;
}

template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayTree<KEY,VALUE>::findBinary(KEY key){
	SplayNode<KEY,VALUE>*t=m_root;
	while(t!=NULL){
		if(t->m_key==key){
			return t;
		}else if(key<t->m_key){
			t=t->m_left;
		}else{
			t=t->m_right;
		}
	}
	return NULL;
}

/*
 * binarySearch(x), if it is not there, return NULL
 * splay(x)
 * return the root
 */
template<class KEY,class VALUE>
SplayNode<KEY,VALUE>*SplayTree<KEY,VALUE>::find(KEY key,bool frozen){
	if(frozen){
		return findBinary(key);
	}

	if(m_root==NULL)
		return NULL;
	splay(key);
	if(m_root->getKey()!=key)
		return NULL;
	return m_root;
}

template<class KEY,class VALUE>
void SplayTree<KEY,VALUE>::print(){
	cout<<"digraph{"<<endl;
	MyStack<SplayNode<KEY,VALUE>*> vertices;
	MyStack<int> depths;
	if(m_root!=NULL){
		vertices.push(m_root);
		depths.push(0);
	}
	while(vertices.size()>0){
		SplayNode<KEY,VALUE>*u=vertices.top();
		vertices.pop();
		int d=depths.top();
		depths.pop();
		cout<<"\""<<u->getKey()<<"\""<<endl;

		if(u->getLeft()!=NULL){
			vertices.push(u->getLeft());
			cout<<"\""<<u->getKey()<<"\""<<" -> "<<"\""<<u->getLeft()->getKey()<<"\""<<endl;
			depths.push(d+1);
		}

		if(u->getRight()!=NULL){
			vertices.push(u->getRight());
			cout<<"\""<<u->getKey()<<"\""<<" -> "<<"\""<<u->getRight()->getKey()<<"\""<<endl;
			depths.push(d+1);
		}
	}
	cout<<"}"<<endl;
}

template<class KEY,class VALUE>
void SplayTree<KEY,VALUE>::splay(KEY key){
	SplayNode<KEY,VALUE> header;
	SplayNode<KEY,VALUE>*l;
	SplayNode<KEY,VALUE>*r;
	SplayNode<KEY,VALUE>*t;
	SplayNode<KEY,VALUE>*y;
	l=r=&header;
	t=m_root;
	header.m_left=header.m_right=NULL;
	while(1){
		if(key<t->m_key){
			if(t->m_left==NULL)
				break;
			if(key<t->m_left->m_key){
				y=t->m_left;
				t->m_left=y->m_right;
				y->m_right=t;
				t=y;
				if(t->m_left==NULL)
					break;
			}
			r->m_left=t;
			r=t;
			t=t->m_left;
		}else if(t->m_key<key){
			if(t->m_right==NULL)
				break;
			if(t->m_right->m_key<key){
				y=t->m_right;
				t->m_right=y->m_left;
				y->m_left=t;
				t=y;
				if(t->m_right==NULL)
					break;
			}
			l->m_right=t;
			l=t;
			t=t->m_right;
		}else{
			break;
		}
	}
	l->m_right=t->m_left;
	r->m_left=t->m_right;
	t->m_left=header.m_right;
	t->m_right=header.m_left;
	m_root=t;
}

#endif
