CC = g++
CFLAGS = -g -Wall -Wno-deprecated
MKDEP=makedepend -Y
OS := $(shell uname)
ifeq ($(OS), Darwin)
  LIBS = -framework OpenGL -framework GLUT
else
  LIBS = -lGL -lGLU -lglut
endif

BINS = gbnimg gbndb
HDRS = socks.h netimg.h imgdb.h ltga.h fec.h
SRCS = ltga.cpp netimglut.cpp socks.cpp
HDRS_SLN = 
SRCS_SLN = netimg.cpp imgdb.cpp
OBJS = $(SRCS:.cpp=.o) $(SRCS_SLN:.cpp=.o)

all: $(BINS)

gbnimg: netimg.o socks.o netimglut.o netimg.h
	$(CC) $(CFLAGS) -o $@ $< netimglut.o socks.o $(LIBS)

gbndb: imgdb.o socks.o ltga.o $(HDRS)
	$(CC) $(CFLAGS) -o $@ $< ltga.o socks.o

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean
clean: 
	-rm -f -r $(OBJS) *.o *~ *core* $(BINS)

depend: $(SRCS_SLN) $(HDRS_SLN) Makefile
	$(MKDEP) $(CFLAGS) $(SRCS_SLN) $(HDRS_SLN) >& /dev/null

# DO NOT DELETE
