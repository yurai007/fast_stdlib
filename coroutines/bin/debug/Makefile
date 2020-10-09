CXX := clang++
CXXFLAGS = -g -std=c++20 -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic -stdlib=libc++
CXXFLAGS += -fsanitize=undefined -fsanitize-recover=address -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
LDFLAGS += -lpthread
LDFLAGS += -ldl -lrt
clang: coro.cc
	$(CXX) $(CXXFLAGS) coro.cc -o coro $(LDFLAGS)

clean:
	@- $(RM) coro

distclean: clean

