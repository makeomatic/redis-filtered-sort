SHELL := /bin/sh
BABEL := ./node_modules/.bin/babel
THIS_FILE := $(lastword $(MAKEFILE_LIST))

BUILD_DIR := ./lib
SOURCES := index.js

$(BUILD_DIR)/%.js: %.js
		$(BABEL) $*.js -d $@

clean:
	rm -rf $(BUILD_DIR)

build: $(foreach src, $(SOURCES), $(BUILD_DIR)/$(src))

all: clean build

redis-module:
	make -C csrc/ build

