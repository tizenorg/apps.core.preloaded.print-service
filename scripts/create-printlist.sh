#!/usr/bin/env bash
pushd "$(dirname $0)" >&2> /dev/null;
if ! [ "$?" -eq "0" ]; then
	printf "[ERROR] Can't execute pushd command\n";
	exit 1;
fi

source base.lib
source search.lib

popd >&2> /dev/null;
if ! [ "$?" -eq "0" ]; then
	printf "[ERROR] Can't execute popd command\n";
	exit 1;
fi

function print-help {
	printf "$0: [hp|epson|samsung] FILE\n"
	printf "Creates list of supported printers for specific vendor\n"
}

function check-arguments {
	local _ARG1="$1"
	if [ -z "$_ARG1" ];
	then
		print-help;
		exit 1;
	fi
}

check-arguments "$1"
check-arguments "$2"
create-file "$2"

case "$1" in
	"hp") supp-filt-hp-modelnames "print" > "$2" ;;
	"epson") filtered-epson-modelnames "print" > "$2" ;;
	"samsung") filtered-sam-modelnames "print" > "$2" ;;
	*) echo "Unrecognized option: $1. Possible variants: hp, epson, samsung"  ;;
esac