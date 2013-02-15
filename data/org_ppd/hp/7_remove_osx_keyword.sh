#!/bin/sh
# http://www.cups.org/documentation.php/doc-1.6/spec-ppd.html

osx_keyword="
APAutoSetupTool
APSupportsCustomColorMatching
APCustomColorMatchingName
APCustomColorMatchingProfile
APDefaultCustomColorMatchingProfile
APDialogExtension
APDuplexRequiresFlippedMargin
APHelpBook
APICADriver
APPrinterIconPath
APPrinterLowInkTool
APPrinterPreset
APPrinterUtilityPath
APScannerOnly
APScanAppBundleID
"
for i in `ls *.ppd`
do
	for keyword in $osx_keyword
	do
		sed -i "/\*[%]*$keyword/d" $i
	done
done
