INCLUDE=/usr/local/include
LIB=/usr/local/lib

CFLAGS	=	-g -I$(INCLUDE)

default:	all

all: ogrinfo

ogrinfo: ogrinfo.cpp
	$(CC) $(CFLAGS) ogrinfo.cpp -L$(LIB) -lgdal -o ogrinfo


