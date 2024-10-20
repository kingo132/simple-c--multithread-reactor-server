# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lpthread

# Platform detection
UNAME_S := $(shell uname -s)

# Source files
SRCS = server.cpp log_manager.cpp client_manager.cpp ring_queue.cpp \
       protocol_handler.cpp tcp_handler.cpp udp_handler.cpp configuration_manager.cpp \
       daemon_manager.cpp dll_functions.cpp ring_queue.cpp select_dispatcher.cpp \
       main.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Target executable
TARGET = mulserver

# Platform-specific flags and sources
ifeq ($(UNAME_S), Linux)
    CXXFLAGS += -D__linux__
    SRCS += epoll_dispatcher.cpp
    LDFLAGS += -ldl   # For dynamically loading libraries in Linux
else ifeq ($(UNAME_S), Darwin)
    CXXFLAGS += -D__APPLE__
    SRCS += select_dispatcher.cpp
endif

# Include directories
INCLUDES = -I.

# Default rule
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Rule for compiling C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
