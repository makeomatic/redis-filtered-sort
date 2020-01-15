include ../variables.include

SOURCEDIR = src
BUILD_DIR ?= ./build

CPP_SOURCES = $(shell find $(SOURCEDIR) -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:$(SOURCEDIR)/%=$(BUILD_DIR)/%.o)

INC_DIRS := $(shell find $(SOURCEDIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I$(SOURCEDIR)
CPPFLAGS := $(CPPFLAGS) $(INC_FLAGS)

.PHONY: all

all: filter_module.so

filter_module.so: $(REDIS_OBJECTS) $(CPP_OBJECTS)
	$(LD) -o $@ $(CPP_OBJECTS) $(REDIS_OBJECTS) $(SHOBJ_LDFLAGS) -lstdc++ -lgcc

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
