# Root Makefile for ptpd, used for cutting releases
RM = /bin/rm

VERSION = ptpd-2.1.1

release:
	(cd src; make clean)
	mkdir $(VERSION)
	(cd $(VERSION); ln -s ../src .; ln -s ../doc .; ln -s ../tools .; ln -s ../extras .)
	tar cvzf $(VERSION).tar.gz -L --exclude .o --exclude Doxygen --exclude .svn --exclude .dep $(VERSION) COPYRIGHT ChangeLog Makefile README RELEASE_NOTES

clean:
	$(RM) -r $(VERSION)
	$(RM) $(VERSION).tar.gz
