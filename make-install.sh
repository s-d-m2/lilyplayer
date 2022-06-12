#!/bin/bash

this_app_name='lilyplayer'


function create_dir()
{
    local readonly dir_to_create="$1"
    mkdir -p "${dir_to_create}"
    if [ ! -d "${dir_to_create}" ] ; then
	printf >&2 'Error could not create directory %s\n' "${dir_to_create}"
	return 2
    fi
}

function install()
{
    if [ $# -eq 0 ] ; then
	printf >&2 'Error install requires a destination folder. In which folder do you want %s to be installed.\n' "${this_app_name}"
	exit 2
    fi

    local readonly dest_dir="$1"

    if [ -z "$dest_dir" ] ; then
	printf >&2 '%s\n' 'Error variable dest_dir is empty'
	exit 2
    fi

    if (! create_dir "${dest_dir}") ; then
	return 2;
    fi

    for path in 'usr/bin/' 'usr/lib/' 'usr/share/applications' 'usr/share/applications/hicolor/256x256/apps/' ; do
	if (! create_dir "${dest_dir}/${path}") ; then
	    return 2;
	fi
    done

    cp -- "./bin/${this_app_name}" "${dest_dir}/usr/bin"
    cp -- './misc/lilyplayer.desktop' "${dest_dir}/usr/share/applications/"
    cp -- './misc/logo_hicolor_256x256.png' "${dest_dir}/usr/share/applications/hicolor/256x256/apps/lilyplayer.png"
}

install "$@"
