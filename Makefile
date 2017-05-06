SHELL := /bin/sh
BABEL := ./node_modules/.bin/babel
THIS_FILE := $(lastword $(MAKEFILE_LIST))

BUILD_DIR := ./lib
SOURCES := index.js filtered-list-bust.lua sorted-filtered-list.lua groupped-list.lua

$(BUILD_DIR)/%.js: %.js
		$(BABEL) $*.js -d $@

$(BUILD_DIR)/%.lua: %.lua
		cp $*.lua $@

clean:
	rm -rf $(BUILD_DIR)

build: $(foreach src, $(SOURCES), $(BUILD_DIR)/$(src))

all: clean build
