MAKEFLAGS+=--no-print-directory
GSOPTS=-q -dSAFER -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -dCompatibilityLevel=1.2 -dUseFlateCompression=true -sOutputFile=$@ -c .setpdfwrite

TARGETS=drvopt
.PHONY: doc clean ${TARGETS}

all: ${TARGETS}

drvopt:
	go build drvopt.go

doc:
	@$(MAKE) -C $@

clean:
	${RM} ${TARGETS}
	$(MAKE) clean -C doc
