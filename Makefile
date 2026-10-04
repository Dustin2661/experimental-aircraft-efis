CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -pedantic -O2
TEST_BINARY ?= /tmp/experimental-aircraft-efis-sl70r-test

.PHONY: test
test:
	$(CXX) $(CXXFLAGS) tests/sl70r_test.cpp -o $(TEST_BINARY)
	$(TEST_BINARY)
