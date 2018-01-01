#
# Makefile
#
CCFLAGS = -I .
LIBPATH =
LIBS = -lstdc++
SRCS = fast_bitstring.cpp main.cpp test.cpp
HDRS = fast_bitstring.h test.h
OBJS = fast_bitstring.o main.o test.o
BIN = fbs
RM=rm -f
CXX=gcc

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CCFLAGS) $(OBJS) $(LIBPATH) $(LIBS) -o $@

.cpp.o: HDRS
	$(CXX) $(CCFLAGS) -c $<

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(BIN)
