CXX := clang++
CXXFLAGS = -g -std=c++2a -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic -stdlib=libc++ -I/home/yurai/programs/executors-impl/include
LDFLAGS += -lpthread
LDFLAGS += -ldl -lrt
clang: coro.cc
	$(CXX) $(CXXFLAGS) coro.cc -o coro $(LDFLAGS)

clean:
	@- $(RM) coro

distclean: clean

