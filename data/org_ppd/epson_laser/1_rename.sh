#!/bin/sh
for i in `ls *.ppd`; do sed -i "s/PCFileName:.*[pP][pP][dD]/PCFileName: \"$i/g" $i; done
