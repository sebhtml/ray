MPICC=mpic++
CXXFLAGS=  -Wall  -O3  -I/usr/include/openmpi/1.2.4-gcc/  -I. #-DDEBUG

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o Edge.o CoverageDistribution.o MessageToSend.o Request.o


%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@


Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

test1: Ray
	mpirun -np 20  -machinefile RayMachinesFile.txt Ray input.txt

test2: Ray
	mpirun -np 20  -machinefile RayMachinesFile.txt Ray input2.txt


test3: Ray
	mpirun -np 10  -machinefile RayMachinesFile.txt Ray input3.txt

clean:
	rm -f *.o Ray
