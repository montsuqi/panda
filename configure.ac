#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([panda],[4.0.0])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([aps/C.c])
AC_CONFIG_HEADERS([config.h])
AC_GNU_SOURCE

# make version
MAJOR_VERSION=`echo AC_PACKAGE_VERSION|sed 's/\(.*\)\.\(.*\)\.\(.*\)/\1/'|sed 's/\([0-9][0-9]*\).*/\1/'`
MINOR_VERSION=`echo AC_PACKAGE_VERSION|sed 's/\(.*\)\.\(.*\)\.\(.*\)/\2/'|sed 's/\([0-9][0-9]*\).*/\1/'`
MICRO_VERSION=`echo AC_PACKAGE_VERSION|sed 's/\(.*\)\.\(.*\)\.\(.*\)/\3/'|sed 's/\([0-9][0-9]*\).*/\1/'`

LTVERSION=`expr $MAJOR_VERSION + $MINOR_VERSION`:$MICRO_VERSION:$MINOR_VERSION

AC_CANONICAL_HOST

SO_SUFFIX=so

case "${host}" in
  *-*-linux* )
  ;;
  *-*-darwin* )
    SO_SUFFIX=dylib
  ;;
esac

AC_DEFINE_UNQUOTED(SO_SUFFIX,"$SO_SUFFIX","")

CONFDIR=`expr $sysconfdir`
AC_SUBST(CONFDIR)

AC_DEFINE_UNQUOTED(PACKAGE,"$PACKAGE","")
AC_DEFINE_UNQUOTED(VERSION,"$VERSION","")
AC_DEFINE_UNQUOTED(CONFDIR,"$CONFDIR","")
AC_DEFINE_UNQUOTED(PACKAGE_DATE, "`date '+%Y%m%d'`","")

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(LTVERSION)

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_INSTALL

am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc

AM_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([dl],[dlopen])
AC_CHECK_LIB([pthread],[pthread_create])
AC_CHECK_LIB([crypt],[crypt])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
if test "$GCC" = "yes"; then
  CFLAGS="$CFLAGS -Wall -Wunused -funsigned-char -Wno-pointer-sign -Wno-unused-result"
fi
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h syslog.h unistd.h crypt.h errno.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT64_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 getcwd gethostbyname gettimeofday memchr memset mkdir munmap pow select socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtol])

# gettext
AC_CANONICAL_HOST
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18.3])

dnl ***************************************************************************
dnl Find pkg-config
dnl ***************************************************************************
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://pkgconfig.sourceforge.net])
fi

dnl ***************************************************************************
dnl checking for server common library
dnl ***************************************************************************
PKG_CHECK_MODULES(COMMON,
[
	uuid		>= 2.17.0
	libmondai 	>= 3.0.0
	librabbitmq	>= 0.4.1
])

dnl ***************************************************************************
dnl checking for amqp library
dnl ***************************************************************************
PKG_CHECK_MODULES(AMQP,
[
	librabbitmq	>= 0.4.1
])

dnl ***************************************************************************
dnl checking for recdefgen
dnl ***************************************************************************
PKG_CHECK_MODULES(RECDEFGEN,
[
	libglade-panda-2.0	>= 2.0.3
	gmodule-2.0			>= 2.0.0
])

dnl ***************************************************************************
dnl glclient Support
dnl ***************************************************************************

AC_ARG_ENABLE(client,
  [  --enable-client [[disable]]],,[enable_client="no"])
if test "x$enable_client" = "xyes"; then
	PKG_CHECK_MODULES(GLCLIENT,
	[
		libglade-panda-2.0	>= 2.0.3
		gtkpanda2			>= 2.0.6
		libcurl             >= 7.0.5
		gmodule-2.0			>= 2.0.0
		libp11				>= 0.2.8
	])

	PKG_CHECK_MODULES(LIBGTKCHECK, libglade-panda-2.0 >= 3.0.0,
	[AC_DEFINE(LIBGTK_3_0_0,1,"")],
	[
		PKG_CHECK_MODULES(LIBGTKCHECK,libglade-panda-2.0 >= 2.0.3,
		[])
	])

	PKG_CHECK_MODULES(CHECK_GTK_VER,gtk+-2.0 >= 2.24.23,
	[
		AC_DEFINE(GTK_2_24_23,1,"")
	], [
		PKG_CHECK_MODULES(CHECK_GTK_VER,gtk+-2.0 >= 2.24.10,
		[])
	])

	PKG_CHECK_MODULES(LWS,
	[
		libwebsockets	>= 1.7.1
	])

	PKG_CHECK_MODULES(CHECK_LWS, libwebsockets >= 2.0.3,
	[
		AC_DEFINE(LWS_2,1,"")
	], [
		PKG_CHECK_MODULES(CHECK_GTK_VER,libwebsockets >= 1.7.1,
		[])
    ])
fi

if test "x$enable_client" = "xyes"
then
	GLCLIENT="glclient"
else
	GLCLIENT=
fi
AC_SUBST(GLCLIENT)

dnl *************************************************************************** 
dnl Add all the cflags & libs needed for the PostgreSQL 
dnl *************************************************************************** 

enable_postgres="no"
AC_ARG_WITH(postgres, 
[  --with-postgres=<directory> use postgres backend in <directory>],[
   AC_MSG_CHECKING(for Postgres files) 
   enable_postgres="yes"
   if test $withval != yes 
   then 
     pgsql_path=$withval 
   fi
])
if test x"$enable_postgres" = "xyes"; then
   if test -z "$pgsql_path"; then
      AC_PATH_PROGS(PG_CONFIG, pg_config, no)
   else 
      AC_PATH_PROGS(PG_CONFIG, pg_config, no, "$pgsql_path/bin")
   fi
   if test x"$PG_CONFIG" = "xno"; then
     AC_MSG_ERROR(Could not find pg_config)
   else
      postgres_bindir=`$PG_CONFIG --bindir`
      postgres_libdir=`$PG_CONFIG --libdir`
      postgres_includedir=`$PG_CONFIG --includedir`
      POSTGRES_LIBS="-L$postgres_libdir -lpq"
      POSTGRES_CFLAGS="-I$postgres_includedir"
      AC_DEFINE(HAVE_POSTGRES,1,"") 
      AC_DEFINE_UNQUOTED(POSTGRES_BINDIR,"$postgres_bindir","")
      POSTGRES_MAJOR_VERSION=`$PG_CONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f1 | sed 's/[[^0-9]]//g'` 
      dnl POSTGRES_MINOR_VERSION=`$PG_CONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f2 | sed 's/[[^0-9]]//g'`
      if test "$POSTGRES_MAJOR_VERSION" -ge 9; then
        AC_DEFINE(POSTGRES_APPLICATIONNAME,1,"")
      fi
   fi
fi
AM_CONDITIONAL(POSTGRESQL, test "x$enable_postgres" = "xyes")
AC_SUBST(POSTGRES_LIBS) 
AC_SUBST(POSTGRES_CFLAGS) 

dnl ***************************************************************************
dnl shell driver
dnl ***************************************************************************

AC_ARG_ENABLE(shell,
    [  --disable-shell         disable shell driver [[enable]]],,[enable_shell="yes"])
if test "x$enable_shell" = "xyes"; then
   AC_DEFINE(USE_SHELL,1,"")
fi

dnl *************************************************************************** 
dnl for the OpenCOBOL
dnl *************************************************************************** 
AC_ARG_ENABLE(opencobol,
  [  --enable-opencobol      enable OpenCOBOL [[disable]]],,[enable_opencobol="no"])
if test "x$enable_opencobol" = "xyes"; then
    AC_MSG_CHECKING(for OpenCOBOL)
    cob_exists=yes
    cob-config --version > /dev/null 2> /dev/null || cob_exists=no
    if test "x$cob_exists" = "xyes"; then
        COB_CFLAGS="`cob-config --cflags`"
        COB_LIBS="`cob-config --libs`"
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_OPENCOBOL,1,"")
    else
        AC_MSG_RESULT(no)
        enable_opencobol="no"
    fi
    AC_SUBST(COB_CFLAGS)
    AC_SUBST(COB_LIBS)
    
    OPENCOBOL_CFLAGS="$COB_CFLAGS"
    OPENCOBOL_LIBS="$COB_LIBS"
    
    AC_SUBST(OPENCOBOL_LIBS) 
    AC_SUBST(OPENCOBOL_CFLAGS) 
fi
AM_CONDITIONAL(OPENCOBOL, test "x$enable_opencobol" = "xyes") 

dnl ***************************************************************************
dnl IP v6
dnl ***************************************************************************

AC_ARG_ENABLE(ipv6,
  [  --enable-ipv6           enable IP v6 [[disable]]],,[enable_ipv6="no"])
if test "x$enable_ipv6" = "xyes"; then
    AC_DEFINE(USE_IPv6,1,"")
fi

dnl ***************************************************************************
dnl SSL
dnl ***************************************************************************

enable_ssl="no"
AC_ARG_WITH(ssl,
	[  --with-ssl=<directory>  use SSL in <directory>],[
	if test x$withval = xyes; then
		PKG_CHECK_MODULES(SSL,[
			libssl >= 0.9.8
			libcrypto
		])
		AC_SUBST(SSL_LIBS) 
		AC_SUBST(SSL_CFLAGS) 
		AC_DEFINE(USE_SSL,1,"")
		PKG_CHECK_MODULES(SSL_CHECK1, libssl >= 1.1.0,
			[],
			[
				PKG_CHECK_MODULES(SSL_CHECK2, libssl < 1.1.0,
					[AC_DEFINE(SSL_1_0_0,1,"")]
				)
			]
		)
		enable_ssl="yes"
	fi
])		

dnl ***************************************************************************
dnl use syslog for message output
dnl ***************************************************************************

AC_ARG_ENABLE(syslog,
  [  --enable-syslog           enable logging to syslog [[disable]]],,[enable_syslog="no"])
if test "x$enable_syslog" = "xyes"; then
    AC_DEFINE(USE_SYSLOG,1,"")
fi

dnl ***************************************************************************
dnl MONTSUQI_LIBRARY_PATH setting
dnl ***************************************************************************

LIB_DIR="${libdir}"
AC_SUBST(LIB_DIR)

AC_ARG_WITH(mon-libpath,
	[  --with-mon-libpath=<DIR>],
	[mon_libpath=$withval])
if test "$mon_libpath" != ""; then
   MONTSUQI_LIBRARY_PATH=$mon_libpath
else
   MONTSUQI_LIBRARY_PATH=$libdir/$PACKAGE
fi
AC_SUBST(MONTSUQI_LIBRARY_PATH)

AC_ARG_WITH(mon-loadpath,
	[  --with-mon-loadpath=<DIR>],
	[mon_loadpath=$withval])
if test "$mon_loadpath" != ""; then
   MONTSUQI_LOAD_PATH=$mon_loadpath
else
   MONTSUQI_LOAD_PATH=$libdir/$PACKAGE
fi
AC_SUBST(MONTSUQI_LOAD_PATH)


dnl ***************************************************************************
dnl ENGINE_PKCS11_PATH
dnl ***************************************************************************
engine_pkcs11="/usr/lib/engines/engine_pkcs11.so"
AC_ARG_WITH(engine_pkcs11,
	[  --with-engine-pkcs11=<file>  use engine_pkcs11.$SO_SUFFIX ],[
	engine_pkcs11=$withval 
])
AC_DEFINE_UNQUOTED(ENGINE_PKCS11_PATH,"$engine_pkcs11","")

AC_CONFIG_FILES([
Makefile 
intl/Makefile 
po/Makefile.in
Doc/Makefile
aps/Makefile
cobol/Makefile
dblib/Makefile
glclient/Makefile
glserver/Makefile
include/Makefile
libs/Makefile
tools/Makefile
wfc/Makefile
panda.pc
])
AC_OUTPUT

dnl ***************************************************************************
dnl output result
dnl ***************************************************************************

echo "------------------------------------------------------"
echo "$PACKAGE Configuration:"
echo ""
echo "  VERSION:               $VERSION"
echo "  glclient support:      $enable_client"
echo "  PostgreSQL support:    $enable_postgres"
echo "  shell driver support:  $enable_shell"
echo "  OpenCOBOL support:     $enable_opencobol"
echo "  IP v6 support:         $enable_ipv6"
echo "  SSL support:           $enable_ssl"
echo "  syslog support:        $enable_syslog"
echo ""
