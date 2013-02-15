#!/bin/bash

PLIST_PATH="/opt/etc/cups/ppd/"
HP_LIST="hp.list"
EPSON_LIST="epson.list"
SAMSUNG_LIST="samsung.list"
HP_DRV="hp/hp.drv"
EPSON_DRV="epson/epson.drv"
SAMSUNG_DRV="samsung/samsung.drv"

function print-pass {
    printf '[\E[32mPASS\E[0m] %s\n' "$1"
}

function print-fail {
    printf '[\E[31mFAIL\E[0m] %s\n' "$1"
}

function print-red {
    printf '\E[31m%s\E[0m' "$1"
}

function print-green {
    printf '\E[32m%s\E[0m' "$1"
}

function print-error {
    printf '[\E[31mERROR\E[0m] %s\n' "$1"
}

function run-opmap-test {
    local _MODEL="$1"
    local _FPATH="$2"
    local _OUTPUT=`getdrv -r "$_MODEL" -i "$_FPATH" 2> /dev/null`
    if [ "$?" -eq "0" ];
    then
        local _PPD=`echo "$_OUTPUT" | sed "s/.*\/\(.*\)\./\1/"`
        #echo "[GETDRV:OK] Model:$_MODEL, PPD file: $_PPD";
        if [ -f "$_PPD" ]
        then
            #echo "PPD PASS"
            local _OUT=$(test-opmap "$_PPD")
            if [ "$?" -eq "0" ]
            then
                print-pass "$_PPD";
                rm -f "$_PPD";
            else
                print-fail "[$_MODEL] $_PPD";
            fi
        else
            print-error "[$_MODEL] getdrv did not extracted file"
        fi
    else
        print-error "getdrv extracting failed on $_PPD"
    fi
}

function run-test {
    local _PLIST="$1";
    local _DRVFILE="$2"
    if [ -f "$_PLIST" ] && [ -f "$_DRVFILE" ]
    then
        while read printer; do
            run-opmap-test "$printer" "$_DRVFILE"
        done < "$_PLIST"
    else
        print-error "file $_PLIST or $_DRVFILE doesn't exist"
    fi
}

function run-test-on-epson {
    run-test "$PLIST_PATH$EPSON_LIST" "$PLIST_PATH$EPSON_DRV"
}

function run-test-on-samsung {
    run-test "$PLIST_PATH$SAMSUNG_LIST" "$PLIST_PATH$SAMSUNG_DRV"
}

function run-test-on-hp {
    run-test "$PLIST_PATH$HP_LIST" "$PLIST_PATH$HP_DRV"
}

function run-all-tests {
    run-test-on-epson;
    run-test-on-samsung;
    run-test-on-hp;
}

function echoerr() { echo "$@" 1>&2; }

function print-help {
    echo "usage: $0 [file]" >&2;
    exit 2;
}

case "$1" in
    "help") print-help ;;
    "-h") print-help ;;
    "--help") print-help ;;
    *) run-all-tests  ;;
esac