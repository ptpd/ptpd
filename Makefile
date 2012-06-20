# Root Makefile for ptpd, used for cutting releases and release candidates

NAME = ptpd
VERSION = ${NAME}-2.2.2
RC = ${NAME}-2-RC-0

rc:
	(cd src; make clean)
	mkdir $(RC)
	(cd $(RC); \
	ln -s ../src .; \
	ln -s ../doc .; \
	ln -s ../tools .; \
	ln -s ../COPYRIGHT .; \
	ln -s ../ChangeLog .; \
	ln -s ../Makefile .; \
	ln -s ../README .; \
	ln -s ../RELEASE_NOTES .)
	tar cvzf $(RC).tar.gz -L --exclude .o --exclude Doxygen --exclude .svn --exclude .dep --exclude core $(RC)

release:
	(cd src; make clean)
	mkdir $(VERSION)
	(cd $(VERSION); \
	ln -s ../src .; \
	ln -s ../doc .; \
	ln -s ../tools .; \
	ln -s ../COPYRIGHT .; \
	ln -s ../ChangeLog .; \
	ln -s ../Makefile .; \
	ln -s ../README .; \
	ln -s ../RELEASE_NOTES .)
	tar cvzf $(VERSION).tar.gz -L --exclude .o --exclude Doxygen --exclude .svn --exclude .dep --exclude core $(VERSION)
