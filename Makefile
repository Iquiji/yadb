.SUFFIXES:
	
CXX = clang -std=c++14 -Wall

MAIN=yadb

SRC=$(filter-out $(MAIN).cpp $(TEST).cpp,$(wildcard *.cpp))
OBJ=$(SRC:.cpp=.o)
HEADERS=$(wildcard *.hpp)

%.o: %.cpp $(HEADERS)
	$(CXX) -c $<

compile: $(MAIN) 

$(MAIN): $(MAIN).o $(OBJ)
	$(CXX) -o $@ $^

format:
	clang-format -i *.[ch]pp

clean:
	rm -rf *.o $(MAIN) $(TEST) test1.ppm

run: compile
	./$(MAIN)

.PHONY: compile clean format test
