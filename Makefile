CC=g++
CDEFINES=
SOURCES=Dispatcher.cpp Mode.cpp precomp.cpp profanity.cpp SpeedSample.cpp TGBot.cpp ResultStore.cpp
OBJECTS=$(SOURCES:.cpp=.o)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    EXECUTABLE=profanity.x64
    LDFLAGS=-framework OpenCL -lcurl -lpthread -lsqlcipher
    CFLAGS=-c -std=c++11 -Wall -O2 -I/usr/include/sqlcipher -I/opt/homebrew/include -I/usr/local/include
else ifeq ($(OS),Windows_NT)
    # MSYS2 / MinGW64 cross-compile on Windows
    EXECUTABLE=profanity.exe
    LDFLAGS=-s -lOpenCL -lcurl -lpthread -lsqlcipher -lws2_32
    CFLAGS=-c -std=c++11 -Wall -O2 -I/usr/include/sqlcipher
else
    EXECUTABLE=profanity.x64
    LDFLAGS=-s -lOpenCL -lcurl -lpthread -lsqlcipher -mcmodel=large
    CFLAGS=-c -std=c++11 -Wall -mmmx -O2 -mcmodel=large -I/usr/include/sqlcipher
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@chmod +x tron 2>/dev/null || true

.cpp.o:
	$(CC) $(CFLAGS) $(CDEFINES) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)

