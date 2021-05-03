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
