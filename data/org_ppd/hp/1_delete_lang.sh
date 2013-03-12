#!/bin/sh
for i in `ls *.ppd`; do sed -i "/\*[a-z][a-z]\./d" $i; done

for i in `ls *.ppd`; do sed -i "/\*zh_CN\./d" $i; done

for i in `ls *.ppd`; do sed -i "/\*zh_TW\./d" $i; done

for i in `ls *.ppd`; do sed -i "/\*cupsLanguages:/d" $i; done
