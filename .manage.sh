#!/bin/bash

function configure() {
	mkdir build &>/dev/null
	cd build
	cmake ..
}

function build() {
	cd build
	cmake --build .
}

function build_lexer() {
	make -C src/re2c
	build
}
