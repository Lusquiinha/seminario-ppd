FLAGS=-O3

CXX=g++

RM=rm -f

EXEC=raytracer

all: $(EXEC)

$(EXEC):
	$(CXX) $(FLAGS) $(EXEC).cpp -c -o $(EXEC).o
	$(CXX) $(FLAGS) $(EXEC).o -o $(EXEC)

clean:
	$(RM) $(EXEC).o $(EXEC)

run: $(EXEC)
	./$(EXEC) < ${EXEC}.in

omp:
	$(CXX) $(FLAGS) $(EXEC)_omp.cpp -o $(EXEC)_omp -fopenmp
