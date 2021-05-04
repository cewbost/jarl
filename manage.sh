#!/bin/bash

jarl_dir=${0%/*}
cd $jarl_dir

until [[ -z "$1" ]]; do
	case "$1" in
		makeimage)
			makeimage=yes
			on=
			;;
		clean)
			clean=yes
			on=
			;;
		config)
			if [[ -z "$config" ]]; then
				config="; _config"
			fi
			on=config
			;;
		make)
			if [[ -z "$make" ]]; then
				make="; _make"
			fi
			on=make
			;;
		run)
			if [[ -z "$run" ]]; then
				run="; _run"
			fi
			on=run
			;;
		*)
			case $on in
				config)
					config="$config $1"
					;;
				make)
					make="$make $1"
					;;
				run)
					run="$run $1"
					;;
				*)
					echo "invalid command: $1"
					exit
					;;
			esac
			;;
	esac
	shift
done

if [[ -n $clean ]]; then
	rm -rf "${jarl_dir}/build"
fi
if [[ -n $makeimage ]]; then
	docker build -t jarl_devel $jarl_dir
fi

cmd="$config""$make""$run"
if [[ -n "$cmd" ]]; then
	docker run -it -v $(pwd):/usr/jarl jarl_devel sh -c "source .manage.sh$cmd"
fi
