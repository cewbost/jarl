basedir = ../

files = $(shell ls | grep .re)
targets = $(addprefix $(basedir)jarl/, $(subst .re,.cpp, $(files)))

re2c_flags = -W -d

.PHONY: all

all: $(targets)

$(basedir)src/%.cpp: $(basedir)re2c/%.re
	re2c $(re2c_flags) -o $@ $<
