#!/bin/bash

jarl_dir=${0%/*}
cd $jarl_dir

function makeimage() {
	docker build -t jarl_devel $jarl_dir
}

function run() {
	docker run -it -v $(pwd):/usr/jarl jarl_devel sh -c "source .manage.sh; $1"
}

case $1 in
	makeimage)
		makeimage ;;
	clean)
		rm -rf "${jarl_dir}/build" ;;
	configure)
		run configure ;;
	build)
		run build ;;
	gcc_version)
		run 'g++ --version' ;;
esac
