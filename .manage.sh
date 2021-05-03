#!/bin/bash

function configure() {
	mkdir build
	cd build
	cmake ..
}

function build() {
	cd build
	make
}

function build_lexer() {
	make -C re2c
	build
}
