include ../variables.include

ifndef RM_INCLUDE_DIR
	RM_INCLUDE_DIR=../lib/
endif

ifndef RMUTIL_LIBDIR
	RMUTIL_LIBDIR=../lib/rmutil
endif

SOURCEDIR = src
BUILD_DIR ?= ./build

CPP_SOURCES = $(shell find $(SOURCEDIR) -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:$(SOURCEDIR)/%=$(BUILD_DIR)/%.o)
#DEPS := $(CPP_OBJECTS:.o=.d)

INC_DIRS := $(shell find $(SOURCEDIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I$(SOURCEDIR)
CPPFLAGS := $(CPPFLAGS) $(INC_FLAGS)

.PHONY: all

all: rmutil filter_module.so

rmutil:
	$(MAKE) -C $(RMUTIL_LIBDIR)

filter_module.so: $(CPP_OBJECTS)
	$(LD) -o $@ $(CPP_OBJECTS) $(SHOBJ_LDFLAGS) -L$(RMUTIL_LIBDIR) -lrmutil -lstdc++ -lgcc

$(BUILD_DIR)/%.cpp.o: $(SOURCEDIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


valgrind:
	valgrind --leak-check=full --show-possibly-lost=no redis-server --loadmodule ./filter_module.so --loglevel debug

debug:
	redis-server --loadmodule ./filter_module.so --loglevel debug

clean:
	$(RM) filter_module.so $(CPP_OBJECTS) $(DEPS)

-include $(DEPS)

FORCE:
