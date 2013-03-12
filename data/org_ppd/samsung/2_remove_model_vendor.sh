#!/bin/sh
for i in `ls *.ppd`; do sed -i "s/\*ModelName:\(.*\)Samsung /\*ModelName:\1/g" $i; done

