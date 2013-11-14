ACLOCAL_AMFLAGS = -I m4

NULL =

SUBDIRS =		\
	src		\
	$(NULL)

EXTRA_DIST =			\
	ChangeLog		\
	README			\
	\
	doc			\
	tools			\
	test			\
	packagebuild		\
	src/ptpd2.conf.minimal\
	src/ptpd2.8\
	src/ptpd2.conf.5\
	\
	$(NULL)

CLEANFILES =
#DISTCLEANFILES = .gcc-warning

BUILT_SOURCES =				\
	libtool				\
	$(NULL)

#dist-hook:
#	@find $(distdir) -type d -name SCCS -print | xargs rm -rf

libtool: $(LIBTOOL_DEPS)
	./config.status --recheck

