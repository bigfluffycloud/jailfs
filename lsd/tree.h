/*
Copyright (c) 2011 Zhehao Mao

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __LIBDS_TREE_H__
#define __LIBDS_TREE_H__

#include <stdlib.h>

enum {
	RED,
	BLACK
};

typedef int (*treecmpfunc)(void*,void*);

struct tree_node {
	struct tree_node * left;
	struct tree_node * right;
	struct tree_node * parent;
	void * data;
	int color;
};

typedef struct tree_node * tnode_p;

tnode_p make_node(void*, int);

struct tree {
	tnode_p root;
	treecmpfunc cmpfunc;
};

typedef struct tree * tree_p;
typedef void (*traversecb)(void*);

tnode_p tree_insert(tree_p tr, void * data, int size);
void tree_delete(tree_p tr, tnode_p node);

void left_rotate(tree_p tr, tnode_p node);
void right_rotate(tree_p tr, tnode_p node);

tnode_p tree_minimum(tnode_p node);
tnode_p tree_maximum(tnode_p node);

tnode_p tree_predecessor(tnode_p node);
tnode_p tree_successor(tnode_p node);

tnode_p tree_search(tree_p tr, void * key);
void traverse(tnode_p node, traversecb);

void destroy_node(tnode_p node);

int rb_color(tnode_p node);
tnode_p rb_insert(tree_p tr, void * data, int size);
void rb_delete(tree_p tr, tnode_p node);

#endif
