SRC=xrwsunpack.c
PROG=xrwsunpack$(EXE)
LDFLAGS += -fsigned-char -Wall

STRIP=strip
MINGW=i586-mingw32msvc
CC=$(MINGW)-gcc -I/usr/$(MINGW)/include -L/usr/$(MINGW)/lib
STRIP=$(MINGW)-strip
LDFLAGS += -s -fomit-frame-pointer -O2
EXE=.exe

all: $(PROG)

xrwsunpack(EXE): $(SRC)
	$(CC) -g -o $@ $^ $(LDFLAGS) -lstdc++ 
	$(STRIP) $@
