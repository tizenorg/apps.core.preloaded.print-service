#!/bin/sh
for i in `ls *.ppd`
do
	sed -i "s/^\*Product:\(.*\)Hewlett-Packard \(.*\)/\*Product: \"\2/g" $i
done

for i in `ls *.ppd`
do
	sed -i "s/^\*Product:\(.*\)([Hh][Pp] \(.*\))/\*Product: \"\2/g" $i
done

for i in `ls *.ppd`
do
	sed -i "s/^\*Product:\(.*\)[Hh][Pp] \(.*\))/\*Product: \"\2/g" $i
done

for i in `ls *.ppd`
do
    num_prd=`cat $i | grep "^*Product" | wc -l`
    num_prd_uniq=`cat $i | grep "^*Product" | sort -u | wc -l`
    if [ "$num_prd" != "$num_prd_uniq" ]
    then
        cat $i | grep "^*Product" | sort -u > temp_prd.txt
        line_nums=`sed -n '/^*Product/=' $i`
        for j in $line_nums
        do
            first_line=$j
            break;
        done
        sed -i '/^*Product/d' $i
        while read line ; do
            sed -i "$first_line i\\$line" $i
        done < temp_prd.txt
        rm temp_prd.txt
    fi
done
