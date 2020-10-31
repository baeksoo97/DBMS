# Et cetera

## How to compile

All source files are written with C++. Therefore, `Makefile` is modified to compile C++ source files and create static library. 

### Project Directory & File Structure
```
|___ project2
     |___ include
     |    |___ api.h
     |    |___ file.h
     |    |___ index.h
     |___ lib
     |    |___ libbpt.a
     |___ main
     |___ src
	  |___ api.cpp
	  |___ api.o
	  |___ file.cpp
	  |___ file.o
	  |___ index.cpp
	  |___ index.o
	  |___ main.cpp
	  |___ main.o
```

### Compile
```
$ make clean
$ make
```