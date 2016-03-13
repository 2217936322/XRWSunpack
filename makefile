SRC=xrwsunpack.c
PROG=xrwsunpack$(EXE)
LDFLAGS += -mlittle-endian -Wall
STRIP=strip

ifeq (${CROSS},win)
STRIP=strip
MINGW=i586-mingw32msvc
CC=$(MINGW)-gcc -I/usr/$(MINGW)/include -L/usr/$(MINGW)/lib
STRIP=$(MINGW)-strip
LDFLAGS += -s -fomit-frame-pointer -O2
EXE=.exe
endif

all: $(PROG)

xrwsunpack$(EXE): $(SRC)
	$(CC) -g -o $@ $^ $(LDFLAGS) -lstdc++ 
	$(STRIP) $@
