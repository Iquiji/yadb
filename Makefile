.SUFFIXES:
	
CXX = clang -std=c11 -Wall -g -fstack-protector-all -fsanitize=address

MAIN=yadb

SRC=$(filter-out $(MAIN).c $(TEST).c,$(wildcard *.c))
OBJ=$(SRC:.c=.o)
HEADERS=$(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CXX) -c $<

compile: $(MAIN) 

$(MAIN): $(MAIN).o $(OBJ)
	$(CXX) -o $@ $^

format:
	clang-format -i *.[ch]pp

clean:
	rm -rf *.o $(MAIN) $(TEST)

run: compile
	./$(MAIN)

.PHONY: compile clean format test
