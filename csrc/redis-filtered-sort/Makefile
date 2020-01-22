include ../variables.include

SOURCEDIR = src
BUILD_DIR ?= ./build

CPP_SOURCES = $(shell find $(SOURCEDIR) -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:$(SOURCEDIR)/%=$(BUILD_DIR)/%.o)
DEPS = $(CPP_OBJECTS:.o=.d)

INC_DIRS = $(shell find $(SOURCEDIR)/ -type d)
BUILD_DIRS = $(INC_DIRS:$(SOURCEDIR)/%=$(BUILD_DIR)/%)

INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(CPPFLAGS) $(INC_FLAGS) -DBOOST_LOG_DYN_LINK

.PHONY: all

all: $(BUILD_DIRS) $(DEPS) filter_module.so

filter_module.so: $(CPP_OBJECTS)
	$(LD) -o $@ $(CPP_OBJECTS) $(REDIS_OBJECTS) $(SHOBJ_LDFLAGS) -lstdc++ -lgcc -lboost_log -lboost_log_setup

$(BUILD_DIRS):
	mkdir -p $(BUILD_DIRS)

$(BUILD_DIR)/%.cpp.o: $(SOURCEDIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.d: $(SOURCEDIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

valgrind:
	valgrind --leak-check=full --show-possibly-lost=no redis-server --loadmodule ./filter_module.so --loglevel debug

debug:
	redis-server --loadmodule ./filter_module.so --loglevel debug

clean:
	$(RM) -rf filter_module.so $(BUILD_DIR)

FORCE:
