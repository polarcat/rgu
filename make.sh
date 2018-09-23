#!/bin/sh

case $0 in
*make-clean.sh)
	set -x
	rm -fvr build lib/build lib/.externalNativeBuild app/build
	;;
*) reset; make-gradle $@
	;;
esac
