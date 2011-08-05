# Ray assembler

Ray is a parallel de novo genome assembler that utilises the message-passing interface everywhere
and is implemented using peer-to-peer communication.

Ray is documented in the manual (InstructionManual.tex; available
online and in Portable Document Format), 
on http://github.com/sebhtml/ray (with the README.md), in the 
C++ code using Doxygen and in the journal paper as well.

## Features & quick facts

- Ray is a high-performance peer-to-peer software.
 - http://dskernel.blogspot.com/2011/07/understand-main-loop-of-message-passing.html

- Ray can be utilised as a k-mer counter.
- Ray builds a k-mer graph (subgraph of a full de Bruijn graph).
- Ray finds paths in this graph.

- Ray is implemented in C++, version ISO/IEC 14882:1998.
- Ray utilises the message-passing interface (MPI), version 2.2.
- Ray supports various network interconnects, thanks to the message-passing interface (MessagesHandler).
- Ray is cloud-ready thanks to the message-passing interface.
 - http://aws.amazon.com/ec2/hpc-applications/

- Ray utilises distributed sparse hash tables with double hashing (MyHashTable).
- Ray utilises smart pointers.
- Ray does garbage collection using real-time memory defragmentation & compaction (DefragmentationGroup).
- Ray utilises a Bloom filter to avoid storing most of the erroneous k-mers in memory.

- Ray utilises virtual communication (VirtualCommunicator).
 - http://dskernel.blogspot.com/2011/01/virtual-communicator.html
 - http://dskernel.blogspot.com/2011/06/more-on-virtual-communication-with.html 

- Ray supports substitution DNA sequencing errors (Illumina).
- Ray supports indels DNA sequencing errors (Pacific Biosciences, 454). <b>In development</b>
- Ray supports paired reads to traverse repeats.
- Ray outputs scaffolds (+ many other useful files).

- Ray is open source with the GNU General Public License.
- Ray is a single executable called Ray (Ray.exe on Microsoft Windows).
- Ray is easy to install and easy to use.

# Website

- http://denovoassembler.sf.net

# Code repository

- http://github.com/sebhtml/ray

# Mailing lists

- Users: denovoassembler-users AT lists.sourceforge.net

- Development/hacking: denovoassembler-devel AT lists.sourceforge.net

- SEQanswers: http://seqanswers.com/forums/showthread.php?t=4301

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

# Outputted files

PREFIX.Contigs.fasta and PREFIX.Scaffolds.fasta

type Ray -help for a full list


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

# Useful links

## Cloud computing

- http://aws.amazon.com/ec2/hpc-applications/
- https://cloud.genomics.cn/
- http://szdaily.sznews.com/html/2011-08/04/content_1689998.htm
- http://www.nature.com/nbt/journal/v28/n7/full/nbt0710-691.html

## Message-passing interface

- http://dskernel.blogspot.com/2011/07/understand-main-loop-of-message-passing.html
- http://cw.squyres.com/
- http://blogs.cisco.com/performance/
- http://www.parawiki.org/index.php/Message_Passing_Interface#Peer_to_Peer_Communication

# Funding

Doctoral Award to S.B., Canadian Institutes of Health Research (CIHR)

# Authors

## Coding, maintenance, implementing ideas:

- Sébastien Boisvert, Doctoral student, Université Laval
http://boisvert.info
http://github.com/sebhtml
http://twitter.com/sebhtml
http://raytrek.com

## Mentoring:

- Prof. François Laviolette, Université Laval
- Prof. Jacques Corbeil, Université Laval


## Design insights for parallel architecture:

- Élénie Godzaridis


## Contributors

- David Eccles
