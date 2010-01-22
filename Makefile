MPICC=~/software/openmpi-1.4.1/output/bin/mpic++
#MPICC=mpic++
MPIRUN=~/software/openmpi-1.4.1/output/bin/mpirun -v
#MPIRUN=mpirun
CXXFLAGS=  -Wall -g -I. -I~/software/openmpi-1.4.1/output/include/ -I~/software/openmpi-1.4.1/output/include/openmpi 

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o Edge.o CoverageDistribution.o Message.o Request.o


%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@


Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

test1: Ray
	$(MPIRUN) -np 10  -machinefile RayMachinesFile.txt Ray input.txt

test2: Ray
	$(MPIRUN) -np 10  -machinefile RayMachinesFile.txt Ray input2.txt


test3: Ray
	$(MPIRUN) -np 10  -machinefile RayMachinesFile.txt Ray input3.txt

clean:
	rm -f *.o Ray
