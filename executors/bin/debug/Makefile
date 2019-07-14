clang: CC := clang++
clang: CXXFLAGS = -Wall -g -std=c++2a -Wno-c99-extensions -Wno-c++98-compat-pedantic -stdlib=libc++ -I/home/yurai/programs/executors-impl/include
clang: LDFLAGS += -lpthread
clang: CXXFLAGS += -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
clang: ../../src/executors.cpp ../../src/std_executors.cpp
	$(CC) $(CXXFLAGS) ../../src/executors.cpp -o executors $(LDFLAGS)
	$(CC) $(CXXFLAGS) ../../src/std_executors.cpp -o std-executors $(LDFLAGS)

gcc: CC := g++
gcc: CXXFLAGS = -Wall -W -Wextra -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -g -std=c++2a -fconcepts -I/home/yurai/programs/executors-impl/include
gcc: LDFLAGS += -lpthread
gcc: CXXFLAGS += -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
gcc: ../../src/executors.cpp ../../src/std_executors.cpp
	$(CC) $(CXXFLAGS) ../../src/executors.cpp -o executors $(LDFLAGS)
	$(CC) $(CXXFLAGS) ../../src/std_executors.cpp -o std-executors $(LDFLAGS)

clean:
	@- $(RM) executors std-executors

distclean: clean


