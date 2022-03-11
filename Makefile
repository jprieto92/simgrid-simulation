BIN_FILES  =  mm3

INSTALL_PATH = $(HOME)/simgrid-3.17

CC = gcc

CPPFLAGS = -I$(INSTALL_PATH)/include #-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

PEDANTIC_PARANOID_FREAK =       -O0 -g -Wshadow -Wcast-align \
				-Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
				-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
				-Wmissing-noreturn -Wredundant-decls -Wnested-externs \
				-Wpointer-arith -Wwrite-strings -finline-functions

REASONABLY_CAREFUL_DUDE =	-Wall -Wextra -O0 -g3 -ggdb3
NO_PRAYER_FOR_THE_WICKED =	-w -O3

LDFLAGS = -L$(INSTALL_PATH)/lib/
LDLIBS = -lm -lsimgrid -rdynamic $(INSTALL_PATH)/lib/libsimgrid.so -Wl,-rpath,$(INSTALL_PATH)/lib


all: CFLAGS=$(NO_PRAYER_FOR_THE_WICKED)
all: $(BIN_FILES)
.PHONY : all

coverage : CFLAGS=$(REASONABLY_CAREFUL_DUDE) -fprofile-arcs -ftest-coverage
coverage : LDLIBS+=-lgcov
coverage : $(BIN_FILES)
.PHONY : coverage

debug : CFLAGS=$(REASONABLY_CAREFUL_DUDE)
debug : $(BIN_FILES)
.PHONY : debug

pedantic : CFLAGS=$(PEDANTIC_PARANOID_FREAK)
pedantic : $(BIN_FILES)
.PHONY : pedantic

mm3: mm3.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

clean:
	rm -f $(BIN_FILES) *.o

.SUFFIXES:
.PHONY : clean
