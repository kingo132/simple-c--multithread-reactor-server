CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall
TARGET = tcp_test_client

all: $(TARGET)

$(TARGET): main.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
