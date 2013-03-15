#!/bin/sh

#used in extraction test
hp_drvgz_path="/usr/share/cups/ppd/hp/hp.drv.gz"
samsung_drvgz_path="/usr/share/cups/ppd/samsung/samsung.drv.gz"
epson_drvgz_path="/usr/share/cups/ppd/epson/epson.drv.gz"

hp_drv_path="/opt/etc/cups/ppd/hp/hp.drv"
samsung_drv_path="/opt/etc/cups/ppd/samsung/samsung.drv"
epson_drv_path="/opt/etc/cups/ppd/epson/epson.drv"

hp_list_path="/opt/etc/cups/ppd/hp.list"
hp_product_path="/opt/etc/cups/ppd/hp_product.list"
samsung_list_path="/opt/etc/cups/ppd/samsung.list"
epson_list_path="/opt/etc/cups/ppd/epson.list"

epson_extraction_complete=0
samsung_extraction_complete=0
hp_extraction_complete=0

EPSON_TEMP="/tmp/epson_temp"
SAMSUNG_TEMP="/tmp/samsung_temp"
HP_TEMP="/tmp/hp_temp"
HP_PRODUCT_TEMP="/tmp/hp_product_temp"

#used in validation test
EPSON_VALID="/tmp/epson_valid"
SAMSUNG_VALID="/tmp/samsung_valid"
HP_VALID="/tmp/hp_valid"
ready_to_test=0

function extraction_test(){

	if [ -f $DRVGZ_PATH ]
	then
		rm $DRV_PATH 2>/dev/null
		cp $DRVGZ_PATH $DRV_PATH.gz
		gzip -d $DRV_PATH.gz
	fi

	if [ -f $DRV_PATH ]
	then
		echo "Extracting ppd from $DRV_PATH"
		exit_val=0
		org_ppd_files=0
		ext_ppd_files=0
	else
		echo "[ERROR] Not found $DRV_PATH"
		exit_val=1
	fi

	if [ $exit_val -eq 0 ]
	then
		if [ ! -d $TEMP_PATH ];then
			mkdir $TEMP_PATH
		else
			rm -rf $TEMP_PATH
			mkdir $TEMP_PATH
		fi
		cd $TEMP_PATH
		#org_ppd_files=`grep -rn " ModelName" $DRV_PATH | wc -l`
		#grep -rn " ModelName" $DRV_PATH | awk -F "ModelName " '{print $2}' | awk -F "\"" '{print $2}' > $TEMP_PATH/modelnames.txt
		org_ppd_files=`cat $LIST_PATH | wc -l`
		cat $LIST_PATH > $TEMP_PATH/modelnames.txt
		starttime=$(/bin/date +%s)
		while read line ; do
			getppd -r "$line" -i $DRV_PATH 2>>$TEMP_PATH/$VENDOR"_error.txt" 1>/dev/null
			if [ ! $? -eq 0 ]
			then
				echo "[FAIL] Failed to extract $line"
				echo "[FAIL] Failed to extract $line" >> $TEMP_PATH/$VENDOR"_error.txt"
			else
				echo "[PASS] $line" >>$TEMP_PATH/$VENDOR"_error.txt"
			fi
		done < $TEMP_PATH/modelnames.txt
		ext_ppd_files=`ls *.ppd | wc -l 2>/dev/null`
		echo "[INFO] Original ppd files = $org_ppd_files"
		echo "[INFO] Extracted ppd files = $ext_ppd_files"
		stoptime=$(/bin/date +%s)
		runtime=$(($stoptime-$starttime))
		echo "Elapsed Time: $runtime sec"
		cd
	fi

	if [ $org_ppd_files -eq $ext_ppd_files ] && [ $org_ppd_files -ne 0 ]
	then
		extraction_complete=1
		echo "[PASS] Checking whether all ppds are extracted"
	else
		extraction_complete=0
		echo "[FAIL] Checking whether all ppds are extracted"
		echo "[INFO] Refer $TEMP_PATH for details"
	fi
}

function epson_extraction_test(){

echo "===================================================================="
echo "	Extraction TEST - Epson							"
echo "===================================================================="

DRVGZ_PATH="$epson_drvgz_path"
DRV_PATH="$epson_drv_path"
LIST_PATH="$epson_list_path"
TEMP_PATH="$EPSON_TEMP"
VENDOR="epson"

extraction_test

if [ $extraction_complete -eq 1 ]
then
	epson_extraction_complete=1
else
	epson_extraction_complete=0
fi

}

function samsung_extraction_test(){
echo "===================================================================="
echo "	Extraction TEST - Samsung							"
echo "===================================================================="

DRVGZ_PATH=$samsung_drvgz_path
DRV_PATH=$samsung_drv_path
LIST_PATH="$samsung_list_path"
TEMP_PATH=$SAMSUNG_TEMP
VENDOR="samsung"

extraction_test

if [ $extraction_complete -eq 1 ]
then
	samsung_extraction_complete=1
else
	samsung_extraction_complete=0
fi

}

function hp_extraction_test(){
echo "===================================================================="
echo "	Extraction TEST - HP								"
echo "===================================================================="

DRVGZ_PATH=$hp_drvgz_path
DRV_PATH=$hp_drv_path
LIST_PATH="$hp_list_path"
TEMP_PATH=$HP_TEMP
VENDOR="hp"

extraction_test

if [ $extraction_complete -eq 1 ]
then
	hp_extraction_complete=1
else
	hp_extraction_complete=0
fi

}

function hp_product_extraction_test(){
echo "===================================================================="
echo "	Extraction TEST - HP Product extraction				"
echo "	*PPD files can be duplicate becasue one ppd support many products."
echo "===================================================================="

DRVGZ_PATH=$hp_drvgz_path
DRV_PATH=$hp_drv_path
PRODUCT_PATH=$hp_product_path
TEMP_PATH=$HP_PRODUCT_TEMP
VENDOR="hp"

if [ -f $DRVGZ_PATH ]
then
	rm $DRV_PATH 2>/dev/null
	cp $DRVGZ_PATH $DRV_PATH.gz
	gzip -d $DRV_PATH.gz
fi

if [ -f $DRV_PATH ]
then
	echo "Extracting ppd from $DRV_PATH"
	exit_val=0
	org_product_num=0
	ext_product_num=0	
else
	echo "[ERROR] Not found $DRV_PATH"
	exit_val=1
fi

if [ $exit_val -eq 0 ]
then
	if [ ! -d $TEMP_PATH ];then
		mkdir $TEMP_PATH
	else
		rm -rf $TEMP_PATH
		mkdir $TEMP_PATH
	fi
	cd $TEMP_PATH
	#org_product_num=`grep -rn "\"Product\"" $DRV_PATH | wc -l`
	#grep -rn "\"Product\"" $DRV_PATH | awk -F "\"" '{print $6}' | awk -F "(" '{print $2}' | awk -F ")" '{print $1}' > $TEMP_PATH/products.txt
	org_product_num=`cat $PRODUCT_PATH | wc -l`
	cat $PRODUCT_PATH > $TEMP_PATH/products.txt
	starttime=$(/bin/date +%s)
	while read line ; do
		getppd -p "$line" -i $DRV_PATH 2>>$TEMP_PATH/$VENDOR"_product_error.txt" 1>/dev/null
		if [ ! $? -eq 0 ]
		then
			echo "[FAIL] Failed to extract $line"
			echo "[FAIL] Failed to extract $line" >> $TEMP_PATH/$VENDOR"_product_error.txt"
		else
			echo "[SUCCESS] $line" >> $TEMP_PATH/$VENDOR"_product_success.txt"
		fi
	done < $TEMP_PATH/products.txt
	ext_product_num=`cat $TEMP_PATH/$VENDOR"_product_success.txt" | wc -l 2>/dev/null`
	echo "[INFO] Original Product Nums = $org_product_num"
	echo "[INFO] Extracted Product Nums = $ext_product_num"
	stoptime=$(/bin/date +%s)
	runtime=$(($stoptime-$starttime))
	echo "Elapsed Time: $runtime sec"
	cd
fi
}

function is_ignore_keyword(){
IGNORE_KEYWORD="
cupsVersion
driverUrl
ModelName
OpenUI
CloseUI
CloseGroup
JCLOpenUI
JCLCloseUI
VariablePaperSize
"

for index in $IGNORE_KEYWORD
do
	if [ "$1" = "$index" ]
	then
		return 1
	fi
done
return 0
}


function validation_test(){

	if [ $EXTRACTION_COMPLETE -eq 0 ]
	then
		echo -n "Extraction is failed..."
		echo -n "Press f key to continue forcefully or any key to quit..."
		read force_run
	fi

	if [ $EXTRACTION_COMPLETE -eq 1 ] || [ "$force_run" = "f" ]
	then
		echo -n "Specify original ppd path[$ORIGINAL_PPD_PATH]: "
		read original_ppd_path
		if [ -z $original_ppd_path ]
		then
			original_ppd_path=$ORIGINAL_PPD_PATH
		fi

		if [ ! -d $original_ppd_path ]
		then
			echo "Checking whether original ppd exists...[FAIL]"
		else
			echo "Checking whether original ppd exists...[PASS]"
			ext_files=`ls $TEMP_PATH/*.ppd | wc -l 2>/dev/null`
			org_files=`ls $original_ppd_path/*.ppd | wc -l 2>/dev/null`

			echo -n "Checking whether ppd's count is equal..."

			if [ $ext_files -eq $org_files ]
			then
				echo "[PASS]"
			else
				echo "[FAIL]"
			fi

			if [ ! -d $VALID_PATH ];then
				mkdir $VALID_PATH
				mkdir $VALID_RESULT_PATH
			else
				rm -rf $VALID_PATH
				mkdir $VALID_PATH
				mkdir $VALID_RESULT_PATH
			fi

			ls $TEMP_PATH/*.ppd | sort | awk -F "/" '{print $NF}' > $VALID_PATH/ext_list 2>/dev/null
			ls $original_ppd_path/*.ppd | sort | awk -F "/" '{print $NF}' > $VALID_PATH/org_list 2>/dev/null

			diff -urN $VALID_PATH/ext_list $VALID_PATH/org_list > $VALID_PATH/diff_list_result

			if [ -s $VALID_PATH/diff_list_result ]
			then
				echo "There are different name or count between original and extracted ppds"
				echo "Please consult $VALID_PATH/diff_list_result for details"
				echo "diff_list_result is to compare $VALID_PATH/org_list and $VALID_PATH/ext_list"
			fi

			ready_to_test=1
		fi
	fi

	if [ $ready_to_test -ne 1 ]
	then
		ready_to_test=0
	else
		echo "Running validation test..."
		mkdir $VALID_PATH/org
		mkdir $VALID_PATH/opt

		for i in `ls $TEMP_PATH/*.ppd`
		do
			cp $i $VALID_PATH/opt/${i##*/}
		done

		echo "Checking whether there are the pair of original and extracted ppds..."
		for i in `ls $original_ppd_path/*.ppd`
		do
			cp $i $VALID_PATH/org/${i##*/}
			not_found_count=0
			if [ ! -s $VALID_PATH/opt/${i##*/} ]
			then
				#echo "[FAIL] ${i##*/} was not found in $VALID_PATH/opt/"
				echo "[FAIL] ${i##*/} was not found in $VALID_PATH/opt/" >> $VALID_PATH/no_found_list
				not_found_count=`expr $not_found_count + 1`
			fi
		done

		if [ $not_found_count -ne 0 ]
		then
			echo "[INFO] Not found ppd : $not_found_count"
		fi

		echo "Checking diff between original and extracted ppds..."
		total_count=`ls $VALID_PATH/org/*.ppd | wc -l`
		processed_count=0
		skipped_count=0
		different_count=0
		passed_count=0

		# *Keyword: Value Type
		KV_keyword=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9-]\+:" | awk -F ":" '{print $1}' | awk -F "*" '{print $2}' | sort -u`
		KV_count=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9-]\+:" | awk -F ":" '{print $1}' | awk -F "*" '{print $2}' | sort -u | wc -l`

		# *Keyword Value: Value Type
		KVV_keyword=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9]\+ [^/]\+:" | awk -F ":" '{print $1}' | awk '{print $1}' | awk -F "*" '{print $2}' | sort -u`
		KVV_count=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9]\+ [^/]\+:" | awk -F ":" '{print $1}' | awk '{print $1}' | awk -F "*" '{print $2}' | sort -u | wc -l`

		# *Keyword Value/Value: Value Type
		#KV_VV_keyword=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9]\+ .\+/.\+:" | awk -F ":" '{print $1}' | awk '{print $1}' | awk -F "*" '{print $2}' | sort -u`
		#KV_VV_count=`cat $VALID_PATH/org/*.ppd | grep "^*[a-zA-Z0-9]\+ .\+/.\+:" | awk -F ":" '{print $1}' | awk '{print $1}' | awk -F "*" '{print $2}' | sort -u | wc -l`

		for i in `ls $VALID_PATH/org/*.ppd`
		do
			processed_count=`expr $processed_count + 1`
			echo -en "\r[$processed_count/$total_count]"
			if [ ! -s $VALID_PATH/opt/${i##*/} ]
			then
				#echo "[SKIP] There is no ${i##*/} in $VALID_PATH/opt/"
				echo "[SKIP] There is no ${i##*/} in $VALID_PATH/opt/" >> $VALID_PATH/skipped_ppd_list
				skipped_count=`expr $skipped_count + 1`
			else
				for keyword in $KV_keyword
				do
					KV_value_org=`getppdvalue KVM $keyword $VALID_PATH/org/${i##*/} | sort`
					KV_value_opt=`getppdvalue KVM $keyword $VALID_PATH/opt/${i##*/} | sort`

					is_ignore_keyword $keyword
					if [ ! $? -eq 1 ]
					then
						if [ "$KV_value_org" != "$KV_value_opt" ]
						then
							echo "@$keyword@" >> $VALID_RESULT_PATH/${i##*/}
							echo "[org] $KV_value_org" >> $VALID_RESULT_PATH/${i##*/}
							echo "[opt] $KV_value_opt" >> $VALID_RESULT_PATH/${i##*/}
						fi
					fi

				done

				for keyword in $KVV_keyword
				do
					KVV_value_org=`getppdvalue KVM $keyword $VALID_PATH/org/${i##*/} | sort`
					KVV_value_opt=`getppdvalue KVM $keyword $VALID_PATH/opt/${i##*/} | sort`

					is_ignore_keyword $keyword
					if [ ! $? -eq 1 ]
					then
						if [ "$KVV_value_org" != "$KVV_value_opt" ]
						then
							echo "@$keyword@" >> $VALID_RESULT_PATH/${i##*/}
							echo "[org] $KVV_value_org" >> $VALID_RESULT_PATH/${i##*/}
							echo "[opt] $KVV_value_opt" >> $VALID_RESULT_PATH/${i##*/}
						fi
					fi
				done

				org_options=`getppdvalue OPT $VALID_PATH/org/${i##*/} | sort`
				opt_options=`getppdvalue OPT $VALID_PATH/opt/${i##*/} | sort`
				
				# get option keywords
				for org_option in $org_options
				do
					is_ignore_keyword $org_option
					if [ ! $? -eq 1 ]
					then
						#get option choices
						KV_VV_org_choices=`cat $VALID_PATH/org/${i##*/} | grep "^*$org_option" | awk '{print $2}' | awk -F ":" '{print $1}' | awk -F "/" '{print $1}'`
						#KV_VV_opt_choices=`cat $VALID_PATH/opt/${i##*/} | grep "^*$org_option" | awk '{print $2}' | awk -F ":" '{print $1}' | awk -F "/" '{print $1}'`

						for choice in $KV_VV_org_choices
						do
							KV_VV_value_org=`getppdvalue CHOICE $choice $VALID_PATH/org/${i##*/}`
							KV_VV_value_opt=`getppdvalue CHOICE $choice $VALID_PATH/opt/${i##*/}`

							#compare choice value
							if [ "$KV_VV_value_org" != "$KV_VV_value_opt" ]
							then
								echo "@$org_option@$choice@" >> $VALID_RESULT_PATH/${i##*/}
								echo "[org] $KV_VV_value_org" >> $VALID_RESULT_PATH/${i##*/}
								echo "[opt] $KV_VV_value_opt" >> $VALID_RESULT_PATH/${i##*/}
							fi
						done
					fi
				done

				if [ -s  $VALID_RESULT_PATH/${i##*/} ]
				then
					echo "[DIFF] ${i##*/}" >> $VALID_PATH/different_ppd_list
					different_count=`expr $different_count + 1`
				else
					echo "[PASS] ${i##*/}" >> $VALID_PATH/equal_ppd_list
					if [ -e  $VALID_RESULT_PATH/${i##*/} ]
					then
						rm $VALID_RESULT_PATH/${i##*/}
					fi
					passed_count=`expr $passed_count + 1`
				fi
			fi
		done

		valid_result=`cat $VALID_RESULT_PATH/*.ppd | grep "^@" | awk -F "@" '{print $2}' | sort -u`
		if [ "$valid_result" != "" ]
		then
			echo "There are keywords including different values"
			echo "Please check files in $VALID_RESULT_PATH for details"
			echo "$valid_result"
		fi

		echo "[INFO] Skipped count : $skipped_count"
		echo "[INFO] Different count : $different_count"
		echo "[INFO] Passed count : $passed_count"

	fi

}

function epson_validation_test(){

echo "===================================================================="
echo "	Validation TEST - Epson							"
echo "===================================================================="

EXTRACTION_COMPLETE=$epson_extraction_complete
ORIGINAL_PPD_PATH="/opt/etc/cups/ppd/org_ppd/epson"
VALID_PATH=$EPSON_VALID
VALID_RESULT_PATH=$EPSON_VALID/result
TEMP_PATH=$EPSON_TEMP
VENDOR="epson"

if [ $EXTRACTION_COMPLETE -eq 1 ]
then 
	echo "Checking whether extraction is completed : [PASS]"
else
	echo "extraction may not be complete."
	echo "Press f key to continue forcefully or any key to extract firstly"
	read go_valid_test

	if [ ! "$go_valid_test" = "f" ]
	then
		echo "Running extraction test..."
		epson_extraction_test
		EXTRACTION_COMPLETE=$epson_extraction_complete
	else
		EXTRACTION_COMPLETE=1
	fi
fi

ready_to_test=0
validation_test

}

function samsung_validation_test(){

echo "===================================================================="
echo "	Validation TEST - Samsung							"
echo "===================================================================="

EXTRACTION_COMPLETE=$samsung_extraction_complete
ORIGINAL_PPD_PATH="/opt/etc/cups/ppd/org_ppd/samsung"
VALID_PATH=$SAMSUNG_VALID
VALID_RESULT_PATH=$SAMSUNG_VALID/result
TEMP_PATH=$SAMSUNG_TEMP
VENDOR="samsung"

if [ $EXTRACTION_COMPLETE -eq 1 ]
then 
	echo "Checking whether extraction is completed : [PASS]"
else
	echo "extraction may not be complete."
	echo "Press f key to continue forcefully or any key to extract firstly"
	read go_valid_test

	if [ ! "$go_valid_test" = "f" ]
	then
		echo "Running extraction test..."
		samsung_extraction_test
		EXTRACTION_COMPLETE=$samsung_extraction_complete
	else
		EXTRACTION_COMPLETE=1
	fi
fi

ready_to_test=0
validation_test

}

function hp_validation_test(){

echo "===================================================================="
echo "	Validation TEST - HP							"
echo "===================================================================="

EXTRACTION_COMPLETE=$hp_extraction_complete
ORIGINAL_PPD_PATH="/opt/etc/cups/ppd/org_ppd/hp"
VALID_PATH=$HP_VALID
VALID_RESULT_PATH=$HP_VALID/result
TEMP_PATH=$HP_TEMP
VENDOR="hp"

if [ $EXTRACTION_COMPLETE -eq 1 ]
then 
	echo "Checking whether extraction is completed : [PASS]"
else
	echo "extraction may not be complete."
	echo "Press f key to continue forcefully or any key to extract firstly"
	read go_valid_test

	if [ ! "$go_valid_test" = "f" ]
	then
		echo "Running extraction test..."
		hp_extraction_test
		EXTRACTION_COMPLETE=$hp_extraction_complete
	else
		EXTRACTION_COMPLETE=1
	fi
fi

ready_to_test=0
validation_test

}

function epson_ui_validation_test(){

echo "===================================================================="
echo "	UI Validation TEST - Epson						"
echo "===================================================================="

echo "TBD..."

}

function samsung_ui_validation_test(){

echo "===================================================================="
echo "	UI Validation TEST - Samsung					"
echo "===================================================================="

echo "TBD..."

}

function hp_ui_validation_test(){

echo "===================================================================="
echo "	UI Validation TEST - HP							"
echo "===================================================================="

echo "TBD..."

}

function help(){
	echo "===================================================================="
	echo " Extraction TEST									"
	echo " -Test whether ppd files are extracted from drv file	"
	echo " -Error log is saved in /tmp/XXX_temp				"
	echo " -It needs ppdc utility, which CUPS includes			"
	echo "												"
	echo " PPD Validation TEST								"
	echo " -Test whether extracted file is valid on the basis of PPD Spec v4.3"
	echo " -It needs cupstestppd utility, which CUPS includes	"
	echo " -If original ppd path is specified,					"
	echo "   It compares the results between original and extracted in sorted"
	echo "   You have to run script in original_ppd path(like 1_rename.sh)"
	echo " UI Validation TEST(TBD)							"
	echo " - Test whether ppd option mapping is valid			"
	echo "===================================================================="
}

function print_menu(){
	echo "===================================================================="
	echo " Extraction TEST								"
	echo "	0. All extraction test(run 1,2,3)				"
	echo "	1. Epson extraction test					"
	echo "	2. Samsung extraction test					"
	echo "	3. HP extraction test						"
	echo "	4. HP Product extraction test				"
	echo " PPD Validation TEST							"
	echo "	5. All PPD validation test(run 6,7,8)			"
	echo "	6. Epson PPD validation test					"
	echo "	7. Samsung PPD validation test				"
	echo "	8. HP PPD validation test					"	
	echo " UIConstraint TEST(TBD)						"
	echo "	9. All UI validation test(run a,b,c)			"
	echo "	a. Epson UI validation test					"
	echo "	b. Samsung UI validation test				"
	echo "	c. HP UI validation test						"
	echo "	h. help									"
	echo "	q. quit									"
	echo "===================================================================="
	echo -n "Which Test :"
}

function quit_menu(){
	echo ""
	echo -n "Press any key to continue or q to quit..."
}

while :
do
	print_menu
	read menu_num
	case $menu_num in
	"0" )
		epson_extraction_test
		samsung_extraction_test
		hp_extraction_test
		hp_product_extraction_test
		exit 0;;
	"1" )
		epson_extraction_test;;
	"2" )
		samsung_extraction_test;;
	"3" )
		hp_extraction_test;;
	"4" )
		hp_product_extraction_test;;
	"5" )
		epson_validation_test
		samsung_validation_test
		hp_validation_test
		exit 0;;
	"6" )
		epson_validation_test;;
	"7" )
		samsung_validation_test;;	
	"8" )	
		hp_validation_test;;
	"9" )	
		epson_ui_validation_test		
		samsung_ui_validation_test
		hp_ui_validation_test;;		
	"a" )
		epson_ui_validation_test;;
	"b" )
		samsung_ui_validation_test;;	
	"c" )	
		hp_ui_validation_test;;
	"h" )
		help;;
	"q" )
		exit 0;;
	esac

	quit_menu
	read quit_num
	case $quit_num in
	"q" )
		exit 0;;
	esac	
done

