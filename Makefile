FLAGS=`pkg-config --cflags --libs jsoncpp gtest` -lextism -lpthread
TEST_FLAGS=$(FLAGS) `pkg-config --cflags --libs gtest`

build-example:
	$(CXX) -std=c++14 -o example -I. example.cpp $(FLAGS)
	
.PHONY: example
example: build-example
	./example

build-test:
	$(CXX) -std=c++14 -o test/test -I. test/test.cpp $(TEST_FLAGS)

.PHONY: test
test: build-test
	cd test && ./test

