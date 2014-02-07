# -std=gnu11?
CC = /usr/bin/gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -std=c11 -O2
DBG_CFLAGS = -Wall —g Werror -Wextra -pedantic -std=c11 -O2

libsrc := openhash64.c 
#Files that are only necessary in executables
exesrc := lametest.c

libdep := $(patsubst %.c,%.d,$(libsrc))
exedep := $(patsubst %.c,%.d,$(exesrc))

libobj := $(patsubst %.c,%.o,$(libsrc))
exeobj := $(patsubst %.c,%.o,$(exesrc))

# debug
libdobj := $(patsubst %.c,%.go,$(libsrc))
exedobj += $(patsubst %.c,%.go,$(exesrc))

# pull in dependency info
-include $(libdep)
-include $(exedep)

all: $(libdep) $(exedep) $(libobj) $(exeobj)
	$(CC) $(CFLAGS) -o lametest $(exeobj) $(libobj)
#-liconv
test.dbg: $(libdep) $(exedep) $(libdobj) $(exedobj)
	$(CC) $(DBG_CFLAGS) -o lametest.dbg $(exedobj) $(libdobj) 

#dependences
%.d:%.c
	$(CC) -MM $(CFLAGS) $*.c > $*.d

# various compilers & flags
# amd64 release
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
#amd64 debug
%.go: %.c
	$(CC) -c $(DBG_CFLAGS) $*.c -o $*.go

clean:
	rm -f hashlametest.dbg hashlametest hashlametest.exe \
 *.o *.go *.d
