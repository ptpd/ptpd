ACLOCAL_AMFLAGS = -I m4

NULL =

SUBDIRS =		\
	src/libcck	\
	src		\
	$(NULL)

EXTRA_DIST =			\
	ChangeLog		\
	README			\
	\
	doc			\
	tools			\
	test			\
	pkgspec			\
	src/ptpd.conf.minimal\
	src/ptpd.8\
	src/ptpd.conf.5\
	src/libcck/INSTALL\
	src/libcck/ChangeLog\
	src/libcck/README\
	src/libcck/COPYRIGHT\
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

