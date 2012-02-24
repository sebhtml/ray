/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

/*

[sboisver12@colosse1 2012-01-25]$ ls
Genome-to-Taxon.tsv  Taxon-Names.tsv  Taxon-Types.tsv  TreeOfLife-Edges.tsv

3. for each sequence, also add an extra color for its genome identifier using a distinct namespace
4. Color things

1. Use the vertices to get a list of identifiers.
2. With this list, load only the relevant pairs from Genome-to-Taxon.tsv. (use an iterator)
x. generate a list of relevant taxon identifiers

y. iteratively load the tree of life (using an iterator-like approach) and fetch things to complete paths to root

5. For each vertex, get the best guess in the tree
	for instance if a k-mer has 3 things on it, try to find a common ancestor in the tree
6. synchronize the tree with master
7. output BiologicalAbundances/_Phylogeny/Hits.tsv
*/

#ifndef _PhylogenyViewer_h
#define _PhylogenyViewer_h

#include <core/ComputeCore.h>
#include <plugins/CorePlugin.h>

/** 
 * a plugin to know what is present in a sample 
 */
class PhylogenyViewer: public CorePlugin{

public:

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

};

#endif

