# Ray assembler

Ray is a parallel de novo genome assembler that utilises the message-passing interface everywhere
and is implemented using peer-to-peer communication.

Ray is implemented using the Ray Platform, a message-passing-interface programming framework.

Ray is documented in

- Documentation/  (many files)
- MANUAL_PAGE.txt  (command-line options, same as Ray -help)
- README.md  (general)
- INSTALL.txt  (quick installation)

## Solutions (all bundled in a single Product called Ray)

- de novo genome assembly (original Ray Technology, re-branded to géno-Ray Technology)
- quantification of contig abundance (con-Ray Technology)

- de novo transcriptome assembly (trans-Ray Technology)
- quantification of transcript expression (express-Ray Technology)

- de novo meta-genome assembly (méta-Ray Technology)
- quantification of microbiome consortia members (éco-Ray Technology)


see Documentation/BiologicalAbundances.txt

# Website

- http://denovoassembler.sf.net

# Code repository

- http://github.com/sebhtml/ray

- http://github.com/sebhtml/RayPlatform

- To get a ChangeLog:

	./scripts/dump-ChangeLog.sh > ChangeLog
	less ChangeLog

# Mailing lists

- Users: denovoassembler-users AT lists.sourceforge.net

- Read it on gmane: http://blog.gmane.org/gmane.science.biology.ray-genome-assembler

- Development/hacking: denovoassembler-devel AT lists.sourceforge.net

- SEQanswers: http://seqanswers.com/forums/showthread.php?t=4301

# Installation

You need a C++ compiler (supporting C++ 1998), make, an implementation of MPI (supporting MPI 2.2).

## Compilation

	tar xjf Ray-x.y.z.tar.bz2
	cd Ray-x.y.z
	make PREFIX=build
	make install
	ls build

## Change the compiler

	make PREFIX=build2000 MPICXX=/software/openmpi-1.4.3/bin/mpicxx
	make install

Tested C++ compilers: see Documentation/COMPILERS.txt

## Use large k-mers

	make PREFIX=Ray-Large-k-mers MAXKMERLENGTH=64
	# wait
	make install
	mpirun -np 512 Ray-Large-k-mers/Ray -k 63 -p lib1_1.fastq lib1_2.fastq \
	-p lib2_1.fastq lib2_2.fastq -o DeadlyBug,Assembler=Ray,K=63
	# wait
	ls DeadlyBug,Assembler=Ray,K=63/Scaffolds.fasta

## Compilation options

	make PREFIX=build-3000 MAXKMERLENGTH=64 HAVE_LIBZ=y HAVE_LIBBZ2=y \
	ASSERT=n FORCE_PACKING=y
	# wait
	make install
	ls build-3000

see the Makefile for more.


# Run Ray

To run Ray on paired reads:

	mpirun -np 25 Ray -p lib1.left.fasta lib1.right.fasta -p lib2.left.fasta lib2.right.fasta -o RayOutput
	ls RayOutput/Contigs.fasta
	ls RayOutput/Scaffolds.fasta
	ls RayOutput/

# Outputted files

RayOutput/Contigs.fasta and RayOutput/Scaffolds.fasta

type Ray -help for a full list of options and outputs


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

see AUTHORS

# Compile Ray on Microsoft Windows with Microsoft Visual Studio

see Documentation/VISUAL_STUDIO.txt




