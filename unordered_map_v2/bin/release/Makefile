gcc: CXX := g++
gcc: CXXFLAGS = -Wall -W -Wextra -Wshadow -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -std=c++20 -fconcepts
gcc: CXXFLAGS += -Ofast -march=native
gcc: LDFLAGS += 
gcc: ../../src/speed_tests.cc
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cc -o speed_tests $(LDFLAGS)

clang: CXX := clang++
clang: CXXFLAGS = -g -Wall -std=c++20 -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic #-stdlib=libc++
clang: CXXFLAGS += -Ofast -march=native
clang: LDFLAGS += 
clang: ../../src/speed_tests.cc 
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cc -o speed_tests_cl $(LDFLAGS)

clean:
	@- $(RM) speed_tests speed_tests_cl

distclean: clean

