clang: CC := clang++
clang: CXXFLAGS = -g -std=c++2a -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic -stdlib=libc++ -I/home/yurai/programs/executors-impl/include
clang: LDFLAGS += -lpthread
#clang: CXXFLAGS += -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
# TO DO: with sanitizers bug in backend? fatal error: error in backend: Cannot represent a difference across section 
clang: coro.cc
	$(CC) $(CXXFLAGS) coro.cc -o coro $(LDFLAGS)

clean:
	@- $(RM) coro

distclean: clean

