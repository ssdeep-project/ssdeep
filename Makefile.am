ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libfuzzy.la
libfuzzy_la_SOURCES = fuzzy.c edit_dist.c
libfuzzy_la_LDFLAGS = -no-undefined -version-info 3:0:1
fuzzy_dll_SOURCES_ = $(srcdir)/fuzzy.c $(srcdir)/edit_dist.c

bin_PROGRAMS = ssdeep
ssdeep_SOURCES = \
	main.cpp match.cpp engine.cpp filedata.cpp               \
	dig.cpp cycles.cpp helpers.cpp ui.cpp edit_dist.h        \
	main.h fuzzy.h tchar-local.h ssdeep.h filedata.h match.h \
	find-file-size.c sum_table.h
ssdeep_LDADD = libfuzzy.la

include_HEADERS = fuzzy.h edit_dist.h

man_MANS = ssdeep.1

dll: $(fuzzy_dll_SOURCES_)
	$(CC) $(CFLAGS) -shared -o fuzzy.dll $(fuzzy_dll_SOURCES_) \
		-Wl,--output-def,fuzzy.def,--out-implib,libfuzzy.a
	$(STRIP) fuzzy.dll

CLEANFILES = fuzzy.dll fuzzy.def

EXTRA_DIST = $(man_MANS) bootstrap sample.c FILEFORMAT m4/README

WINDOWSDOCS = README.TXT API.TXT FILEFORMAT.TXT NEWS.TXT

README.TXT: $(srcdir)/ssdeep.1
	COLUMNS=76 man $< | col -bx > $@

API.TXT: $(srcdir)/README
	cp $< $@

FILEFORMAT.TXT: $(srcdir)/FILEFORMAT
	cp $< $@

NEWS.TXT: $(srcdir)/NEWS
	cp $< $@

win-docs: $(WINDOWSDOCS)

win-package: win-docs
	rm -rf $(distdir).zip $(distdir)
	$(MAKE)
	$(MAKE) dll
	$(STRIP) ssdeep.exe
	mkdir $(distdir)
	cp $(WINDOWSDOCS) ssdeep.exe fuzzy.dll fuzzy.def $(srcdir)/fuzzy.h $(srcdir)/sample.c $(distdir)
	zip -lr9 $(distdir).zip $(distdir)
	rm -rf $(distdir) $(WINDOWSDOCS)

world: distclean
	$(srcdir)/configure --host=i686-w64-mingw32 LIBS="-Wl,-Bstatic -lstdc++ -lpthread -static-libgcc"
	$(MAKE) win-package
	$(MAKE) dist
