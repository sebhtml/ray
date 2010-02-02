MPICC=~/software/openmpi-1.4.1/output/bin/mpic++
MPIRUN=~/software/openmpi-1.4.1/output/bin/mpirun 
CXXFLAGS=  -Wall -O3 -I.

# colosse:
MPIRUN=/software/MPI/openmpi-1.4.1_gcc/bin/mpirun
MPICC=/software/MPI/openmpi-1.4.1_gcc/bin/mpic++

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o Edge.o CoverageDistribution.o Message.o 


%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@


Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

test: Ray
	$(MPIRUN) -np 28  -machinefile RayMachinesFile.txt Ray input.txt

test1: Ray
	$(MPIRUN) -np 30  -machinefile RayMachinesFile.txt Ray input1.txt|tee test1.log

test2: Ray
	$(MPIRUN)   -np 30  -machinefile RayMachinesFile.txt Ray input2.txt|tee test2.log
	echo see test2.log


test3: Ray
	$(MPIRUN) -np 28  -machinefile RayMachinesFile.txt Ray input3.txt

clean:
	rm -f *.o Ray
