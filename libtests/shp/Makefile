INCLUDE=/usr/local/include/libshp
LIB=/usr/local/lib

CFLAGS	=	-g -I$(INCLUDE)

default:	all

all: shpdump

shpdump: shpdump.cc
	$(CC) $(CFLAGS) shpdump.cc -L$(LIB) -lshp -o shpdump


