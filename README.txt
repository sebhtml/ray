Description:

Ray is a single-executable program. Its aim is to assemble sequences on
mpi-enabled computers or clusters.
Ray is implemented in c++. It is tested with OpenMPI and g++ on Linux 2.6.
Only the master rank needs to access the files on disk.

Availability:

http://denovoassembler.sourceforge.net/


Author:

Sébastien Boisvert, PhD student, Université Laval.
Sebastien.Boisvert.3@ulaval.ca
http://boisvert.info/


Examples of input files:

input2.txt  input3.txt  input.txt  RayInputTemplate.txt


Example of machines file:

RayMachinesFile.txt


Example of command:

mpirun -np 1 -machinefile RayMachinesFile.txt input.txt  # 1 cpu only

mpirun -np 1024  -machinefile RayMachinesFile.txt input.txt  # 1024 cpu

mpirun -np 4  -machinefile RayMachinesFile.txt input.txt  # quad-core


Limitations:

Ray can not run in non-mpi run as it is a pure parallel program with no
non-parallel steps. However, it is safe and reliable to run it with 1 cpu.

