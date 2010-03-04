= Acknowledgments =

We are very thankful to the [https://www.clumeq.ca/ CLUMEQ consortium] for access to [http://www.top500.org/site/systems/3088 colosse], a 7680-slot computer. Our hardware infrastructure is mostly funded by the [http://www.innovation.ca/en Canada Foundation for Innovation]. [http://boisvert.info/ Sébastien Boisvert] has a scholarship from the [http://www.cihr-irsc.gc.ca/e/193.html Canadian Institutes of Health Research].
[http://www.cri.ulaval.ca/?jcorbeil_eng Jacques Corbeil] is the [http://genome.ulaval.ca/corbeillab Canada Research Chair in Medical Genomics] and is funded by the [http://www.cihr-irsc.gc.ca/e/193.html Canadian Institutes of Health Research].
[http://www.cs.ucl.ac.uk/people/F.Laviolette.html François Laviolette] is funded by [http://www.nserc-crsng.gc.ca/ Natural Sciences and Engineering Research Council of Canada]. We thank [https://sourceforge.net/ SourceForge] for the project hosting.

= Ray: a massively parallel MPI-based approach to de Bruijn genome assembly with mixed technologies =

Ray is a parallel genome assembler utilizing [http://en.wikipedia.org/wiki/Message_Passing_Interface MPI]. 
Ray is a single-executable program (the executable is Ray). Its aim is to assemble sequences on
mpi-enabled computers or clusters.
Ray is implemented in c++. It is tested with OpenMPI and g++ on Linux 2.6.
Only the master rank needs to access the files on disk.

== Features ==

Ray

* is massively parallel,
* supports the mixing of sequencing technologies, as long as the error incorporation is random,
* supports paired-end reads,
* does not write pesky intermediate large binary files,
* is licensed with the [http://www.gnu.org/licenses/gpl-3.0.html GPL version 3],
* is implemented in c++,
* distributes computation across MPI processes,
* distributes data across MPI processes,
* is tested with [http://open-mpi.org/ Open-MPI],
* runs on [http://www.linux.com/ GNU/Linux],
* uses bit shifts and other optimizations,
* supports a vertex size up to 31,
* uses splay trees, and
* runs on one and more MPI processes.

== How to cite us ==

 Sébastien Boisvert, Jacques Corbeil, and François Laviolette. 
 Ray: a massively parallel MPI-based approach to de Bruijn genome assembly with mixed technologies. 
 http://denovoassembler.sf.net/, 2010.

== Installation ==

=== Download Ray! ===

[https://sourceforge.net/projects/denovoassembler/files/ Download Ray] (version 0.0.3 will be available around 10 March 2010)

=== Compilation ===

 tar -xjf Ray-0.0.3.tar.bz2
 cd Ray-0.0.3
 make

To use an alternative mpic++ executable:

 make MPICC=/home/boiseb01/software/ompi-1.4.1-gcc/bin/mpic++

=== Example of input files === 

RayInputTemplate.txt

=== Examples of commands ===

 mpirun -np 1 -machinefile RayMachinesFile.txt input.txt  # 1 cpu only

 mpirun -np 4  -machinefile RayMachinesFile.txt input.txt  # quad-core

 mpirun -np 1024  -machinefile RayMachinesFile.txt input.txt  # 1024 cpu

== Community ==

* Mailing list: denovoassembler-users <AT> lists <DOT> sourceforge <DOT> net ([http://sourceforge.net/mailarchive/forum.php?forum_name=denovoassembler-users archives])

== Limitations ==

Ray can not run without mpi as it is a pure parallel program with no
non-parallel steps. However, it is safe and reliable to run it with 1 cpu.

It currently works well with Open-MPI.
You may have to disable the Byte Transfer Layer called Shared Memory to avoid a race condition in the spinlock of Open-MPI. This problem is presumably due to a processor defective design rather than a defect in Open-MPI. The lock instruction is supposed to lock the bus, so if you encounter the race condition, you know something is wrong. The following command disables the btl sm.

 mpirun -np 32 --mca btl ^sm ./Ray sequences.fasta



= OpenAssembler: assembling genome with mixed sequencing technologies =

<b>OpenAssembler</b> assembles Illumina reads or 454 + Illumina reads, or any combination without non-random error incorporation. The manuscript is under review. Its novelty is that it avoids both 454's homopolymers and Illumina's short read length. Unlike EULER-SR, Velvet, or ABySS, OpenAssembler can assemble reads obtained from a mix of technology. However, it runs only on one process. 

=== Manuscript ===

We submitted a manuscript on 15 October 2009. The source code will be available upon acceptance.

[https://sourceforge.net/projects/denovoassembler/files/ Download OpenAssembler] (not there yet, sorry)

