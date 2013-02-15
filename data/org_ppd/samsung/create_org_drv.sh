#!/bin/sh
mkdir temp
cd temp
cp ../*.ppd ./
cp ../*.sh ./
./1_rename.sh
./2_remove_model_vendor.sh
ppdi -o samsung_org.drv ./*.ppd
cp ./samsung_org.drv ../../samsung_org.drv
rm -rf temp/
cd ..
