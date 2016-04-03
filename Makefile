CC = g++ 
CFLAGS = -g -Wall -Wno-deprecated $(DASGN)
MKDEP=makedepend -Y
OS := $(shell uname)
ifeq ($(OS), Darwin)
  LIBS = -framework OpenGL -framework GLUT
else
  LIBS = -lGL -lGLU -lglut
endif

BINS = rdpimg rdpdb
HDRS = ltga.h socks.h fec.h
SRCS = ltga.cpp netimglut.cpp socks.cpp fec.cpp
HDRS_SLN = netimg.h imgdb.h
SRCS_SLN = netimg.cpp imgdb.cpp 
OBJS = $(SRCS:.cpp=.o) $(SRCS_SLN:.cpp=.o)

all: $(BINS)

rdpimg: netimg.o netimglut.o fec.o socks.o $(HDRS)
	$(CC) $(CFLAGS) -o $@ $< netimglut.o fec.o socks.o $(LIBS)

rdpdb: imgdb.o ltga.o fec.o socks.o $(HDRS)
	$(CC) $(CFLAGS) -o $@ $< ltga.o fec.o socks.o
	
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean
clean: 
	-rm -f -r $(OBJS) *.o *~ *core* rdpimg $(BINS)

depend: $(SRCS_SLN) $(HDRS_SLN) Makefile
	$(MKDEP) $(CFLAGS) $(SRCS_SLN) $(HDRS_SLN) >& /dev/null

# DO NOT DELETE

netimg.o: netimg.h
imgdb.o: netimg.h imgdb.h
imgdb.o: netimg.h
