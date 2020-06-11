gcc: CXX := g++
gcc: CXXFLAGS = -Wall -W -Wextra -Wshadow -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -g -std=c++20 -fconcepts
gcc: CXXFLAGS += -fstack-protector -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
gcc: LDFLAGS += 
gcc: ../../src/speed_tests.cc
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cc -o speed_tests $(LDFLAGS)

clang: CXX := clang++
clang: CXXFLAGS = -Wall -Wpedantic -g -std=c++20 -fcoroutines-ts -Wno-c99-extensions -Wno-c++98-compat-pedantic #-stdlib=libc++
clang: CXXFLAGS += -fsanitize=address -fsanitize=undefined -fsanitize-recover=address -fsanitize-address-use-after-scope -fsanitize=signed-integer-overflow -fsanitize=vptr
clang: ../../src/speed_tests.cc
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cc -o speed_tests_cl $(LDFLAGS)

heavy: 
	make CXXFLAGS='-Wall -W -Wextra -Wpedantic -Wformat-security -Walloca -Wduplicated-branches -g -std=c++20 -fconcepts' -j4 gcc && \
		valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --expensive-definedness-checks=yes ./speed_tests

clean:
	@- $(RM) speed_tests speed_tests_cl

