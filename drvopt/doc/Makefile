GSOPTS=-q -dSAFER -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -dCompatibilityLevel=1.2 -dUseFlateCompression=true -sOutputFile=$@ -c .setpdfwrite
PS=graph1.ps graph2.ps graph3.ps graph4.ps graph5.ps graph6.ps 
CLEANFILES=drvopt.pdf drvopt.ps ${PS}

doc: drvopt.pdf

drvopt.pdf: drvopt.ps
	gs ${GSOPTS} -f $<

drvopt.ps: drvopt.1 ${PS}
	groff -Tps -P-pa4 -mwww -ms -mpspic $< > $@

graph1.ps: graph1.gv
	dot -Teps $< > $@

graph2.ps: graph2.gv
	dot -Teps $< > $@

graph3.ps: graph3.gv
	dot -Teps $< > $@

graph4.ps: graph4.gv
	dot -Teps $< > $@

graph5.ps: graph5.gv
	dot -Teps $< > $@

graph6.ps: graph6.gv
	dot -Teps $< > $@

clean:
	${RM} ${CLEANFILES}
