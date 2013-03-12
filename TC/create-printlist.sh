#!/bin/bash

CURRENT_PATH=`pwd`
SEARCH_PATH="$2/data/org_ppd"

# Arrange ppd, remove unsupported ppds and rename ppds
function arrange-ppd {
	local _DIR="$SEARCH_PATH/$1"
	echo "$_DIR"
	cd $_DIR
	$_DIR/arrange_ppd.sh
	cd $CURRENT_PATH
}

# Create file with list of printer's ModelNames
function create-printlist {
	local _DIR="$1"
	local _FNAME="$2"
    local _RETURN="1"
    local _DIRLIST=`eval echo $_DIR`
    #find $_DIRLIST -iname "*.ppd" -exec grep "\*ModelName\:" {} \;  | sed 's/.*\"\(.*\)\"/\1/' | sed 's/^\(epson\|hp\|samsung\) \(.*\)/\2/I' > "$_FNAME";
	cat $_DIRLIST/*.ppd | grep "[^a-z]ModelName" | grep "[^a-z]ModelName" | awk -F "\"" '{print $2}' > "$_FNAME"
}

function create-productlist {
	local _DIR="$1"
	local _FNAME="$2"
    local _RETURN="1"
    local _DIRLIST=`eval echo $_DIR`
    #find $_DIRLIST -iname "*.ppd" -exec grep "\*ModelName\:" {} \;  | sed 's/.*\"\(.*\)\"/\1/' | sed 's/^\(epson\|hp\|samsung\) \(.*\)/\2/I' > "$_FNAME";
	cat $_DIRLIST/*.ppd | grep "^*Product" | awk -F "\"" '{print $2}' > "$_FNAME"
}

function create-epson-printlist {
	local _DIR="$1"
	local _FNAME="$2"
	local _ORGDRV="$_DIR"
	arrange-ppd epson
	create-printlist "$_ORGDRV/epson" "$_FNAME"
}

function create-samsung-printlist {
	local _DIR="$1"
	local _FNAME="$2"
	local _ORGDRV="$_DIR"
	arrange-ppd samsung
	create-printlist "$_ORGDRV/samsung" "$_FNAME"
}

function create-hp-printlist {
	local _DIR="$1"
	local _FNAME="$2"
	local _ORGDRV="$_DIR"
	arrange-ppd hp
	create-printlist "$_ORGDRV/hp" "$_FNAME"
}

function create-hp-productlist {
	local _DIR="$1"
	local _FNAME="$2"
	local _ORGDRV="$_DIR"
	create-productlist "$_ORGDRV/hp" "$_FNAME"
}

function create-all-printlist {
	local _DIR="$1"
	local _FNAME="$2"
	create-epson-printlist "$_DIR" "epson.list"
	create-samsung-printlist "$_DIR" "samsung.list"
	create-hp-printlist "$_DIR" "hp.list"
	create-hp-productlist "$_DIR" "hp_product.list"
}

case "$1" in
    "hp") create-hp-printlist "$SEARCH_PATH" "$3" ;;
    "hp_product") create-hp-productlist "$SEARCH_PATH" "$3" ;;
    "epson") create-epson-printlist "$SEARCH_PATH" "$3" ;;
    "samsung") create-samsung-printlist "$SEARCH_PATH" "$3" ;;
    *) echo "Unrecognized option: $1. Possible variants: hp, hp_product, epson, samsung"  ;;
esac