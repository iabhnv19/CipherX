CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Iinclude
LDFLAGS = -lws2_32 -lbthprops

SRCS = src/common.cpp \
       src/ciphers.cpp \
       src/vault.cpp \
       src/cryptanalysis.cpp \
       src/bluetooth.cpp \
       src/actions.cpp \
       src/main.cpp

TARGET = project.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	del /f $(TARGET)

