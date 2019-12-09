CXX := clang++
CXXFLAGS = -g -std=c++2a -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic -stdlib=libc++ -I/home/yurai/programs/executors-impl/include
LDFLAGS += -lpthread
LDFLAGS += -ldl -lrt
#clang: CXXFLAGS += -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
# TO DO: with sanitizers bug in backend? fatal error: error in backend: Cannot represent a difference across section 
clang: coro.cc
	$(CXX) $(CXXFLAGS) coro.cc -o coro $(LDFLAGS)

raft: raft.cc
	$(CXX) $(CXXFLAGS) raft.cc -o raft $(LDFLAGS)

clean:
	@- $(RM) coro raft

distclean: clean

