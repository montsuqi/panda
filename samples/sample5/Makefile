TARGET		=	cal.so

SOURCES		=	cal.c

LDFLAGS		=	-module -export-dynamic
GLIB_CFLAGS	:=	$(shell glib-config --cflags)
FIXED_CFLAGS:=	$(shell libfixed-config --cflags)
MONDAI_CFLAGS:=	$(shell libmondai-config --cflags)

PANDA_DIR	:=	$(shell echo ~/panda)

INCLUDES 			= 		\
	-I$(PANDA_DIR)/include	\
	-I$(PANDA_DIR)/libs		\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

CC = gcc
CFLAGS = -g -O2 -Wall -Wunused 


all: $(TARGET)

%.o: %.c
	$(CC) -fPIC $(CFLAGS) $(INCLUDES) -c $<

%.so: %.o
	$(CC) -shared -o $*.so $<
