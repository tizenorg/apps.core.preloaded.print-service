#!/bin/sh
mkdir temp
cd temp
cp ../*.ppd ./
cp ../*.sh ./
./1_rename.sh
./2_delete_lang.sh
./3_remove_model_vendor.sh
ppdi -o epson_laser_org.drv ./*.ppd
cp ./epson_laser_org.drv ../../epson_laser_org.drv
rm -rf temp/
cd ..
