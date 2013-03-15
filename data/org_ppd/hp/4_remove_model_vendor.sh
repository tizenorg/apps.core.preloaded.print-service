#!/bin/sh
for i in `ls *.ppd`; do sed -i "s/\*ModelName:\(.*\)HP /\*ModelName:\1/g" $i; done

for i in `ls *.ppd`; do sed -i "s/\*ModelName:\(.*\)\"\(.*\) \+\"/\*ModelName:\1\"\2\"/g" $i; done

