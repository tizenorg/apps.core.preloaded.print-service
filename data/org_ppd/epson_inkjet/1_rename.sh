#!/bin/sh
for i in `ls *.ppd`; do sed -i "s/PCFileName:.*PPD/PCFileName:            \"$i/g" $i; done
