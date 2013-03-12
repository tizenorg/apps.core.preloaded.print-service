#!/bin/bash

export INPDFFILE=$1

#find all PPD files recursively from current dir
export PPDFILES=`find ./ -type f -name \*.ppd`
#echo $PPDFILES
for PPD in $PPDFILES; do
	mkdir -p ./results/$PPD
	#get PageSize list from ppd file
	export PAGESIZES=`grep -G "^*PageSize" $PPD|sed "s/^\*PageSize\ \(.[a-zA-Z0-9]*\)\//\1     /" | awk '{print($1);}'|grep -v \*PageSize`

	for PAGESIZE in $PAGESIZES; do
		for ORIENTATION in 3 4; do
			for NUMBERUP in 1 2 4; do
				echo /usr/lib/cups/filter/pdftopdf job user title 1 \"orientation-requested=$ORIENTATION number-up=$NUMBERUP PageSize=$PAGESIZE fit-to-page=True\" $INPDFFILE>./results/$PPD/$PAGESIZE.orient-$ORIENTATION.nup-$NUMBERUP.pdf
				/usr/lib/cups/filter/pdftopdf job user title 1 "orientation-requested=$ORIENTATION number-up=$NUMBERUP PageSize=$PAGESIZE fit-to-page=True" $INPDFFILE>./results/$PPD/$PAGESIZE.orient-$ORIENTATION.nup-$NUMBERUP.pdf
			done
		done
	done
done
