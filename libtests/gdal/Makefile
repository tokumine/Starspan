INCLUDE=/usr/local/include
LIB=/usr/local/lib

CFLAGS	=	-g -I$(INCLUDE)

default:	all

all: gdalinfo

gdalinfo: gdalinfo.cc
	$(CC) $(CFLAGS) gdalinfo.cc -L$(LIB) -lgdal -o gdalinfo


