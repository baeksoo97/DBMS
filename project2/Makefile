.SUFFIXES: .c .o

CC=gcc

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.c
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

CFLAGS+= -g -fPIC -I $(INC)

OBJS=$(SRCDIR)bpt.o $(SRCDIR)file.o $(SRCDIR)main.o
TARGET=main

all:
	make clean
	make compile

compile: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCDIR)$(TARGET).o -L $(LIBS) -lbpt

$(OBJS):
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.c)
	ar cr $(LIBS)libbpt.a $@

clean:
ifneq ("$(wildcard src/bpt.o)","")
	rm src/bpt.o
endif
ifneq ("$(wildcard src/file.o)","")
	rm src/file.o
endif
ifneq ("$(wildcard src/main.o)","")
	rm src/main.o
endif
ifneq ("$(wildcard $(TARGET))","")
	rm $(TARGET)
endif
ifneq ("$(wildcard lib/libbpt.a)","")
	rm lib/libbpt.a
endif

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
