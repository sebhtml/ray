# Ray assembler

Ray is a parallel de novo genome assembler that utilises the message-passing interface everywhere
and is implemented using peer-to-peer communication.

Ray is documented in the manual (InstructionManual.tex; available
online and in Portable Document Format), 
on http://github.com/sebhtml/ray (with the README.md), in the 
C++ code using Doxygen and in the journal paper as well.

Enjoy the README.

Ray is implemented in C++, version ISO/IEC 14882:1998.

It also utilises the message-passing interface (MPI), version 2.2.

# Website

- http://denovoassembler.sf.net

# Code repository

- http://github.com/sebhtml/ray

# Mailing list

- denovoassembler-users AT lists.sourceforge.net

# Installation

You need a C++ compiler, make, an implementation of MPI.

## Compilation

	tar xjf Ray-x.y.z.tar.bz2
	cd Ray-x.y.z
	make PREFIX=build
	make install
	ls build

## Change the compiler

	make PREFIX=build MPICXX=/software/openmpi-1.4.3/bin/mpic++

## Use large k-mers

	make PREFIX=Ray-Large-k-mers MAXKMERLENGTH=64
	# wait
	make install
	mpirun -np 512 Ray-Large-k-mers/Ray -k 63 -p lib1_1.fastq lib1_2.fastq \
	-p lib2_1.fastq lib2_2.fastq -o DeadlyBug,Assembler=Ray,K=63
	# wait
	ls DeadlyBug,Assembler=Ray,K=63.Scaffolds.fasta

## Compilation options

	make PREFIX=build-3000 MAXKMERLENGTH=64 HAVE_LIBZ=y HAVE_LIBBZ2=y \
	HAVE_CLOCK_GETTIME=n INTEL_COMPILER=n ASSERT=n FORCE_PACKING=y
	# wait
	make install
	ls build-3000

see the Makefile for more.


# Run Ray

To run Ray on paired reads:

	mpirun -np 25 Ray -p lib1.left.fasta lib1.right.fasta -p lib2.left.fasta lib2.right.fasta -o prefix
	ls prefix.Contigs.fasta
	ls prefix.Scaffolds.fasta
	ls prefix.*

# Color space

Ray assembles color-space reads and generate color-space contigs.
Files must have the .csfasta extension. Nucleotide reads can not be mixed
with color-space reads. This is an experimental feature.

# Publications

http://denovoassembler.sf.net/publications.html

# Code

## Code documentation

	cd code
	doxygen DoxygenConfigurationFile
	cd DoxygenDocumentation/html
	firefox index.html


## Message-passing interface

- http://cw.squyres.com/
- http://www.parawiki.org/index.php/Message_Passing_Interface#Peer_to_Peer_Communication

# Funding

Doctoral Award to S.B., Canadian Institutes of Health Research (CIHR)

# Authors


## Coding, maintenance, implementing ideas:

- Sébastien Boisvert, Doctoral student, Université Laval
http://boisvert.info
http://twitter.com/sebhtml


## Mentoring:

- Prof. François Laviolette, Université Laval
- Prof. Jacques Corbeil, Université Laval


## Design insights for parallel architecture:

- Élénie Godzaridis

	
