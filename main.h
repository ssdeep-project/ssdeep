// ssdeep
// Copyright (C) 2012 Kyrus
//
// $Id$
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef __MAIN_H
#define __MAIN_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include <errno.h>
#include <limits.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif 

#ifdef HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif


// This allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more.
// Technically it isn't needed in ssdeep as we don't process standard
// input. But it was part of Jesse's template, so in it goes!
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif


#define FALSE  0
#define TRUE   1


#endif   // #ifndef __MAIN_H
