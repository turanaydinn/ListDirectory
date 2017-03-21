# ====================================================================
# 
#     Filename:  Makefike
# 
#     Description:  Makefile for compiling and executing C/C++ files
# 
#     Created:  Wednesday 08 March 2017 14:46:46  IST
#     Compiler:  gcc
# 
#     Author:  Mahmut Ensari Güvengil, <eguvengil@gmail.com>
#     License:  GNU General Public License
#     Copyright:  Copyright (c) 2017, Developers
#                 https://github.com/mahmutguvengil
# 
# ====================================================================
#		     HW02 - List Directory Project Makefile
# ====================================================================

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -c -g  -Wall -pedantic -pedantic-errors 

# the build target executable:
TARGET = withPipeandFIFO

EXE = exe

all: $(TARGET)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c

$(TARGET): $(TARGET).o
	$(CC) -o $(EXE) $(TARGET).o

clean:
	rm $(EXE) $(TARGET).o log.txt

# end of Makefile ----
