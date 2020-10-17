.SUFFIXES: .cpp .o

CC=gcc
CXX=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.c
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

CFLAGS+= -g -fPIC -I $(INC)
CPPFLAGS+= -g -fPIC -I $(INC)

SRCS=$(wildcard $(SRCDIR)*.cpp )
OBJS=$(patsubst %.cpp, %.o, $(notdir $(SRCS)))
OUT_OBJS = $(patsubst %, $(SRCDIR)%, $(OBJS))

TARGET=main

all: $(TARGET)

$(TARGET): $(OUT_OBJS)
	make static_library
	$(CXX) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(OUT_OBJS) $(LIBS)*

library:
	g++ -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OUT_OBJS)

static_library:
	ar cr $(LIBS)libbpt.a $(OUT_OBJS)
