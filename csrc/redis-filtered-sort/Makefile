include ../variables.include

#override path defined in variables
JANSSON_LIBDIR=../lib/jansson

ifndef RM_INCLUDE_DIR
	RM_INCLUDE_DIR=../lib/
endif

ifndef RMUTIL_LIBDIR
	RMUTIL_LIBDIR=../lib/rmutil
endif

SOURCEDIR=$(shell pwd -P)
CC_SOURCES = $(wildcard $(SOURCEDIR)/*.c) 
CC_OBJECTS = $(patsubst $(SOURCEDIR)/%.c, $(SOURCEDIR)/%.o, $(CC_SOURCES))

all: rmutil $(CC_OBJECTS) filter_module.so
rmutil: FORCE
	$(MAKE) -C $(RMUTIL_LIBDIR)

filter_module.so: $(CC_OBJECTS)
	$(LD) -o $@ $(CC_OBJECTS) $(SHOBJ_LDFLAGS) $(LIBS) -L$(RMUTIL_LIBDIR) -L$(JANSSON_LIBDIR)/src/.libs -lrmutil -lpthread -lc -Bstatic -ljansson 

valgrind:
	valgrind --leak-check=full --show-possibly-lost=no redis-server --loadmodule ./filter_module.so --loglevel debug

clean:
	rm -rf *.xo *.so *.o
	cd $(RMUTIL_LIBDIR) && make clean

FORCE:
