
bin_PROGRAMS=ssdeep

ssdeep_LDADD=libfuzzy.la
ssdeep_LDFLAGS=-static

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES=libfuzzy.la
libfuzzy_la_SOURCES=fuzzy.c edit_dist.c find-file-size.c
libfuzzy_la_LDFLAGS=-no-undefined -version-info 3:0:1

include_HEADERS=fuzzy.h edit_dist.h

man_MANS=ssdeep.1

ssdeep_SOURCES = main.cpp match.cpp engine.cpp filedata.cpp   	\
                 dig.cpp cycles.cpp helpers.cpp ui.cpp edit_dist.h     	\
                 main.h fuzzy.h tchar-local.h ssdeep.h filedata.h match.h

dll: $(libfuzzy_la_SOURCES)
	$(CC) $(CFLAGS) -shared -o fuzzy.dll $(libfuzzy_la_SOURCES) \
		-Wl,--output-def,fuzzy.def,--out-implib,libfuzzy.a
	$(STRIP) fuzzy.dll

CLEANFILES=fuzzy.dll fuzzy.def

EXTRA_DIST=$(man_MANS) bootstrap sum_table.h sample.c FILEFORMAT m4/README

WINDOWSDOCS=README.TXT API.TXT FILEFORMAT.TXT NEWS.TXT

README.TXT: ssdeep.1
	COLUMNS=76 man ./ssdeep.1 | col -bx > README.TXT

API.TXT: README
	cp README API.TXT

FILEFORMAT.TXT: FILEFORMAT
	cp FILEFORMAT FILEFORMAT.TXT

NEWS.TXT: NEWS
	cp NEWS NEWS.TXT

win-docs: $(WINDOWSDOCS)
#	flip -d $(WINDOWSDOCS)
#	unix2dos $(WINDOWSDOCS)

win-package: win-docs
	rm -rf $(distdir).zip $(distdir)
	make
	make dll
	$(STRIP) ssdeep.exe
	mkdir $(distdir)
	cp $(WINDOWSDOCS) ssdeep.exe fuzzy.dll fuzzy.def fuzzy.h sample.c $(distdir)
#	flip -d $(distdir)/{sample.c,fuzzy.def}
#	unix2dos $(distdir)/{sample.c,fuzzy.def}
	zip -lr9 $(distdir).zip $(distdir)
	rm -rf $(distdir) $(WINDOWSDOCS)

world: distclean
	./configure --host=i686-w64-mingw32 LIBS="-Wl,-Bstatic -lstdc++ -lpthread -static-libgcc"
	make win-package
	make dist


# Only generic routines go below this line
# ------------------------------------------------------------------

nice:
	rm -f *~

preflight:
	@grep RBF $(DISTFILES)

