ACLOCAL_AMFLAGS = -I m4

NULL =

SUBDIRS =		\
	src		\
	$(NULL)

EXTRA_DIST =			\
	COPYRIGHT		\
	ChangeLog		\
	README.md		\
	\
	doc			\
	tools			\
	test			\
	packagebuild		\
	src/ptpd2.conf.default-full\
	src/ptpd2.conf.minimal\
	src/ptpd2.8\
	src/ptpd2.conf.5\
	src/leap-seconds.list\
	src/dep/iniparser/AUTHORS\
	src/dep/iniparser/LICENSE\
	src/dep/iniparser/README\
	src/templates.conf\
	\
	$(NULL)

dist-hook:
	rm -f $(distdir)/packagebuild/rpm-rh/*.rpm

CLEANFILES =
#DISTCLEANFILES = .gcc-warning

BUILT_SOURCES =				\
	libtool				\
	$(NULL)

ptpd2dir = $(datadir)/$(PACKAGE)
ptpd2_DATA = 	src/leap-seconds.list		\
		src/ptpd2.conf.minimal		\
		src/ptpd2.conf.default-full	\
		src/templates.conf		\
		doc/PTPBASE-MIB.txt		\
		$(NULL)

#dist-hook:
#	@find $(distdir) -type d -name SCCS -print | xargs rm -rf

libtool: $(LIBTOOL_DEPS)
	./config.status --recheck

