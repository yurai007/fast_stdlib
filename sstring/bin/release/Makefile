LDFLAGS = 
SOURCES :=  ../../src/

CXX_SRCS := $(wildcard $(SOURCES)*.cpp)
OBJS := ${CXX_SRCS:.cpp=.o}

.PHONY: all clean distclean

all: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o main $(LDFLAGS)

clang: 
	make CXX=clang++ CXXFLAGS='-std=c++2a -Weverything -Wno-c++98-compat -Wpedantic \
		-Ofast' all

gcc: 
	make CXX=g++ CXXFLAGS='-Wall -W -Wextra -Wshadow -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -std=c++2a -fconcepts \
		-Ofast' all

clean:
	@- $(RM) main
	@- $(RM) $(OBJS)

distclean: clean
