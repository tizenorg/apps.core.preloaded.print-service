#!/bin/sh
for i in `ls *.ppd`; do sed -i "s/\*ModelName:\(.*\)HP /\*ModelName:\1/g" $i; done

