[http://denovoassembler.sf.net/ DeNovoAssembler.SF.Net] hosts the Ray project -- a massively parallel open source genome assembler for sequencers such as [http://454.com/ Roche 454 sequencers], [http://illumina.com/ Illumina sequencers], [http://solid.appliedbiosystems.com/ SOLiD sequencers], [http://www.pacificbiosciences.com/ Pacific Biosciences sequencers], [http://www.helicosbio.com/ Helicos Biosciences sequencers], and exciting [http://www.iontorrent.com/ Ion Torrent] semiconductor-based sequencers. Ray can assemble reads obtained with a mix of sequencing technologies too!


* [http://sourceforge.net/projects/denovoassembler/files/ Download]
* [http://lists.sourceforge.net/lists/listinfo/denovoassembler-users Mailing list]
* [http://seqanswers.com/forums/showthread.php?t=4301 SeqAnswers.com thread]


= Ray: a massively parallel MPI-based approach for genome assembly with mixed technologies =

Ray is a parallel genome assembler utilizing [http://en.wikipedia.org/wiki/Message_Passing_Interface MPI]. 
Ray is a single-executable program (the executable is Ray). Its aim is to assemble sequences on
mpi-enabled computers or clusters.
Ray is implemented in c++.
Only the master rank needs to access the files on disk. 

== Tested technologies ==

* Illumina
* SOLiD 
* mix of technologies (to obtain random noise)

== Supported file formats ==

* Color-Space Fasta (".csfasta")
* Fasta with qualities ("fastq")
* SFF (".sff")
* Fasta (".fasta")

== Reads ==

* single-end reads (with LoadSingleEndReads)
* paired-end reads (with LoadPairedEndReads)

== Other features ==

* is massively parallel
* supports the mixing of sequencing technologies, as long as the error incorporation is random
* is implemented in c++
* distributes computation across MPI processes
* distributes data across MPI processes
* runs on one and more MPI processes

== How to cite us?==

 Sébastien Boisvert, Jacques Corbeil, and François Laviolette. 
 Ray: a massively parallel MPI-based approach for genome assembly with mixed technologies. 
 http://denovoassembler.sf.net/, 2010.

== Installation ==


=== Requirements ===

* make
* g++
* openmpi (1.3.4 or 1.4.1)
* sequence data ;)
* Any GNU/Linux flavor will do.

=== Tested platforms ===

* Fedora
* CentOS
* Ubuntu

=== License ===

* [http://www.gnu.org/licenses/gpl-3.0.html GPL version 3]

[http://sourceforge.net/projects/denovoassembler/files/ Download]

 tar -xjf Ray-<version>.tar.bz2
 cd Ray-<version>
 make

To use an alternative mpic++ executable:

 make MPICC=/home/boiseb01/software/ompi-1.4.1-gcc/bin/mpic++

== Running Ray ==

You must use mpirun:

 mpirun -np 1 -machinefile RayMachinesFile.txt Ray RayInputTemplate.txt  # 1 cpu only
 mpirun -np 4  -machinefile RayMachinesFile.txt Ray RayInputTemplate.txt  # quad-core
 mpirun -np 1024  -machinefile RayMachinesFile.txt Ray RayInputTemplate.txt  # 1024 cpu

Basically, Ray understands two commands in RayInputTemplate.txt:

 LoadSingleEndReads <sequencesFile>

and

 LoadPairedEndReads <leftSequencesFile> <rightSequencesFile> <fragmentLength> <fragmentLengthStandardDeviation>

<leftSequencesFile> and <rightSequencesFile> must contain the exact same number of sequences, paired reads must be on reverse strands, and the <fragmentLength> includes the read lengths. Ray supports fasta, fastq, and sff formats. But beware!, if your sff file contains paired-end reads, you must first extract the information, and tell Ray to use them with LoadPairedEndReads.

Examples:

 LoadSingleEndReads 1.fasta
 LoadSingleEndReads 2.fastq
 LoadSingleEndReads UIJD.sff
 LoadPairedEndReads 908_1.fastq 908_fastq 215 30
 LoadPairedEndReads large_l.fasta large_r.fasta 2000 200

Ray likes the following extensions: ".sff", ".fasta", ".fastq", and ".csfasta".

Ray writes contigs to Contigs.fasta.

== Limitations ==

Ray can not run without mpi as it is a pure parallel program with no
non-parallel steps. However, it is safe and reliable to run it with 1 cpu.
It currently works well with Open-MPI.
You may have to disable the Byte Transfer Layer called Shared Memory to avoid a race condition in the spinlock of Open-MPI. This problem is presumably due to a processor defective design rather than a defect in Open-MPI. The lock instruction is supposed to lock the bus, so if you encounter the race condition, you know something is wrong. The following command disables the btl sm at runtime.

 mpirun -np 32 --mca btl ^sm ./Ray template.ray

= OpenAssembler: assembling genome with mixed sequencing technologies =

OpenAssembler assembles Illumina reads or 454 + Illumina reads, or any combination without non-random error incorporation. The manuscript is under review. Its novelty is that it avoids both 454's homopolymers and Illumina's short read length. Unlike EULER-SR, Velvet, or ABySS, OpenAssembler can assemble reads obtained from a mix of technology. However, it runs only on one process. 

=== References ===

Paper:

We submitted a manuscript on 15 October 2009. The source code of OpenAssembler will be available upon acceptance.

We published an abstract on OpenAssembler in a conference:

 OpenAssembler: assembly of reads from a mix of high-throughput sequencing technologies. 
 Sébastien Boisvert, François Laviolette, Jacques Corbeil. 
 [http://www.centrerc.umontreal.ca/colloque2009a.html Robert Cedergren Bioinformatics Colloquium 2009], 2009.

= Acknowledgments =

Developing an open source assembler is made possible by investing in research infrastructure. We are very thankful to the [http://www.clumeq.ca/ CLUMEQ consortium] for access to [http://www.top500.org/site/systems/3088 colosse], a 7680-slot computer. Our hardware infrastructure is mostly funded by the [http://www.innovation.ca/en Canada Foundation for Innovation].

Scholarships and grants are equally important. [http://boisvert.info/ Sébastien Boisvert] has a scholarship from the [http://www.cihr-irsc.gc.ca/e/193.html Canadian Institutes of Health Research].
[http://www.cri.ulaval.ca/?jcorbeil_eng Jacques Corbeil] is the [http://genome.ulaval.ca/corbeillab Canada Research Chair in Medical Genomics] and is funded by the [http://www.cihr-irsc.gc.ca/e/193.html Canadian Institutes of Health Research].
[http://www.cs.ucl.ac.uk/people/F.Laviolette.html François Laviolette] is funded by [http://www.nserc-crsng.gc.ca/ Natural Sciences and Engineering Research Council of Canada]. 

We thank [http://sourceforge.net/ SourceForge] for the project hosting. We are also thankful to the Free/Libre Open Source Software community for outstanding projects such as [http://www.gnu.org/ GNU], [http://gcc.gnu.org/ g++], [http://www.open-mpi.org/ Open-MPI], [http://kernel.org/ Linux], just to name a few.

