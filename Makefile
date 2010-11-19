# Root Makefile for ptpd, used for cutting releases

VERSION = ptpd-1.1.0

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
