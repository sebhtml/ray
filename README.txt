Installation:

Ray works with OpenMPI 1.4.1.



Description:

Ray is a single-executable program (the executable is Ray). Its aim is to assemble sequences on
mpi-enabled computers or clusters.
Ray is implemented in c++. It is tested with OpenMPI 1.4.1 and g++ on Linux 2.6.
Only the master rank needs to access the files on disk.



Availability:

http://denovoassembler.sourceforge.net/



Installation:

tweak the Makefile (you must provide the path to mpic++)



Author:

Sébastien Boisvert, PhD student, Université Laval.
Sebastien.Boisvert.3@ulaval.ca
http://boisvert.info/



Examples of input files:

input2.txt  input3.txt  input.txt  RayInputTemplate.txt



Example of machines file:

RayMachinesFile.txt



Example of commands (see the Makefile also.)

mpirun -np 1 -machinefile RayMachinesFile.txt input.txt  # 1 cpu only

mpirun -np 1024  -machinefile RayMachinesFile.txt input.txt  # 1024 cpu

mpirun -np 4  -machinefile RayMachinesFile.txt input.txt  # quad-core



Running Ray with Sun Grid Engine (with qsub):

see the file HumanChromosome1-qsub.sh



Limitations:

Ray can not run without mpi as it is a pure parallel program with no
non-parallel steps. However, it is safe and reliable to run it with 1 cpu.
Ray is distributed with the GPLv3 license (see gpl-3.txt).
