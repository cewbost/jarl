#!/bin/bash

function _config() {
	set -e
	mkdir build &>/dev/null
	cd build; cmake ..; cd ..
}

function _make() {
	set -e

	local lexer=""

	while [[ -n "$1" ]]; do
		case $1 in
			lexer)
				lexer=yes ;;
			*)
				echo "make: invalid command $1"
				exit
				;;
		esac
		shift
	done
	
	if [[ -n "$lexer" ]]; then
		make -C src/re2c/
	fi
	
	cd build; cmake --build .; cd ..
}

function _run() {
	set -e
	echo run
	until [[ -z "$1" ]]; do
		echo $1
		shift
	done
}
