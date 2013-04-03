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

#ifndef _OntologyTypes_h
#define _OntologyTypes_h

#include <stdint.h>

/* encodes something like GO:0011221 */
typedef uint32_t GeneOntologyIdentifier;

typedef uint8_t GeneOntologyDomain;

#define GENE_ONTOLOGY_DOMAIN_cellular_component 0x0
#define GENE_ONTOLOGY_DOMAIN_biological_process 0x1
#define GENE_ONTOLOGY_DOMAIN_molecular_function 0x2

#define GENE_ONTOLOGY_DOMAIN_biological_process_STRING "biological_process"
#define GENE_ONTOLOGY_DOMAIN_cellular_component_STRING "cellular_component"
#define GENE_ONTOLOGY_DOMAIN_molecular_function_STRING "molecular_function"


#endif

