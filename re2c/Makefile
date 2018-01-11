basedir = ../

files = $(shell ls -B | grep .re)
targets = $(addprefix $(basedir)src/, $(subst .re,.cpp, $(files)))

re2c_flags = -W -d

.PHONY: all

all: $(targets)

$(basedir)src/%.cpp: $(basedir)re2c/%.re
	re2c $(re2c_flags) -o $@ $<
