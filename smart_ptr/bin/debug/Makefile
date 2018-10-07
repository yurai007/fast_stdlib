CXXFLAGS = -Wall -W -Wextra -Wshadow -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -g -std=c++2a -fconcepts
CXXFLAGS += -fstack-protector -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
LDFLAGS = -lboost_system -lpthread
CXX := g++
SOURCES :=  ../../src/

CXX_SRCS := $(wildcard $(SOURCES)*.cpp)
OBJS := ${CXX_SRCS:.cpp=.o}

.PHONY: all clean distclean

all: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o main $(LDFLAGS)

clean:
	@- $(RM) main
	@- $(RM) $(OBJS)

distclean: clean
