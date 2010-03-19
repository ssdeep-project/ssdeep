# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.
#
# $Id$
#

ALL_FILES = manpage.html index.html usage.html changes.txt style.css api

USERNAME = jessekornblum,ssdeep
DESTDIR  = web.sourceforge.net:htdocs/

RMAN = /sw/bin/rman

preflight:
	rm -f *~
	grep RBF *.html *.txt

manpages:
	$(RMAN) -f HTML -S -l "ssdeep man page" ../ssdeep.1 > manpage.html

changes:
	cp ../NEWS changes.txt

publish:
	rsync -avP -e ssh $(ALL_FILES) $(USERNAME)@$(DESTDIR)

all: manpages changes publish
