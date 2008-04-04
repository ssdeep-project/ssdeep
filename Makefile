
CC       = gcc
CFLAGS   = -Wall -W -O3
GOAL     = ssdeep
LINK_OPT = 

# Where we get installed
PREFIX = /usr/local

# Directory where cross-compiliation tools live (see INSTALL for info)
CR_BASE = $(HOME)/mingw32/mingw32/bin

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------
VERSION = 1.1

# Comment out this line when debugging is done
#CFLAGS += -D_DEBUG -O0 -ggdb

CFLAGS += -DVERSION=\"$(VERSION)\"

SRC = main.c edit_dist.c match.c engine.c helpers.c dig.c cycles.c
OBJ = main.o edit_dist.o match.o engine.o helpers.o dig.o cycles.o
HEADERS = Makefile main.h
DOCS = COPYING CHANGES INSTALL FILEFORMAT $(GOAL).1 

#---------------------------------------------------------------------

all: linux

linux: CFLAGS += -D__LINUX -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
linux: $(GOAL)

unix: $(GOAL)

mac: $(GOAL)

macg5: CFLAGS += -fast
macg5: $(GOAL)

cross: CC = $(CR_BASE)/gcc
cross: CFLAGS += -static
cross: SUFFIX = .exe
cross: LINK_OPT += -liberty
cross: $(GOAL)
	$(CR_BASE)/strip $(GOAL)$(SUFFIX)

$(GOAL): $(OBJ)
	$(CC) $(CFLAGS) -o $(GOAL)$(SUFFIX) $(OBJ) $(LINK_OPT)

$(OBJ): $(HEADERS)

#---------------------------------------------------------------------

install: $(GOAL)
	install -d $(PREFIX)/bin $(PREFIX)/man/man1
	install -m 755 $(GOAL) $(PREFIX)/bin
	install -m 644 $(GOAL).1 $(PREFIX)/man/man1

uninstall:
	rm -f -- $(PREFIX)/bin/$(GOAL) $(PREFIX)/man/man1/$(GOAL).1

#---------------------------------------------------------------------

# Anything in the code that needs further attention is marked with the
# letters 'RBF', which stand for "Remove Before Flight." This is similar
# to how aircraft are marked with streamers to indicate maintaince is
# required before the plane can fly. Typing 'make preflight' will do a
# check for all RBF tags; thus showing the developer what needs to be
# fixed before the program can be released.
preflight:
	@grep RBF $(SRC) *.h $(DOCS)

nice:
	rm -f *~

clean: nice
	rm -f $(OBJ) $(GOAL) $(GOAL).exe $(TAR_FILE).gz $(DEST_DIR).zip
	rm -rf $(DEST_DIR) $(WINDOC)

#---------------------------------------------------------------------
# MAKING PACKAGES
#-------------------------------------------------------------------------
EXTRA_FILES =
DEST_DIR = $(GOAL)-$(VERSION)
TAR_FILE = $(DEST_DIR).tar
PKG_FILES = $(SRC) $(HEADERS) $(DOCS) $(EXTRA_FILES)
WINDOC = FILEFORMAT.txt CHANGES.txt README.txt COPYING.txt
ZIP_FILES = $(GOAL).exe $(WINDOC)

# This packages me up to send to somebody else
package_dir:
	rm -f $(TAR_FILE) $(TAR_FILE).gz
	mkdir $(DEST_DIR)
	cp $(PKG_FILES) $(DEST_DIR)

package: package_dir
	tar cvf $(TAR_FILE) $(DEST_DIR)
	rm -rf $(DEST_DIR)
	gzip $(TAR_FILE)

win-doc:
	man ./$(GOAL).1 | col -bx > README.txt
	cp FILEFORMAT FILEFORMAT.txt
	cp CHANGES CHANGES.txt
	cp COPYING COPYING.txt
	unix2dos $(WINDOC)

ZIP_FLAGS=-r9
internal: ZIP_FLAGS+= -e 
internal: world

world: clean package_dir win-doc cross
	zip $(ZIP_FLAGS) $(DEST_DIR).zip $(GOAL).exe $(WINDOC) $(DEST_DIR)
	rm -rf $(WINDOC) $(DEST_DIR)

