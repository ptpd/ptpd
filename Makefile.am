ACLOCAL_AMFLAGS = -I m4

NULL =

SUBDIRS =		\
	src		\
	$(NULL)

EXTRA_DIST =			\
	ChangeLog		\
	NEWS			\
	README			\
	\
	doc			\
	tools			\
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

