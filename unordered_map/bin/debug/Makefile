CXXFLAGS = -Wall -W -Wextra -Wpedantic -Wshadow -Wformat-security -Walloca -Wduplicated-branches -g -std=c++2a -fconcepts
CXXFLAGS += -fstack-protector -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
LDFLAGS = 
CXX := g++

correctness_tests: ../../src/correctness_tests.cpp
	$(CXX) $(CXXFLAGS) ../../src/correctness_tests.cpp -o correctness_tests $(LDFLAGS)

speed_tests: ../../src/speed_tests.cpp 	
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cpp -o speed_tests $(LDFLAGS)

sstring: ../../src/sstring.cpp
	$(CXX) $(CXXFLAGS) ../../src/sstring.cpp -o sstring $(LDFLAGS)

all: correctness_tests speed_tests sstring

clean:
	rm -rf correctness_tests speed_tests sstring

distclean: clean
