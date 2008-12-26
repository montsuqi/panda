#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([panda],[1.4.3])
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

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(LTVERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

#AM_PROG_CC_STDC
AC_PROG_CC
AC_DIAGNOSE([obsolete],[AM_PROG_CC_STDC:
	your code should no longer depend upon `am_cv_prog_cc_stdc', but upon
	`ac_cv_prog_cc_stdc'.  Remove this warning and the assignment when
	you adjust the code.  You can also remove the above call to
	AC_PROG_CC if you already called it elsewhere.])
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc

AM_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([pandas], [SendChar])
AC_CHECK_LIB([dl],[dlopen])
AC_CHECK_LIB([pthread],[pthread_create])
AC_CHECK_LIB([crypt],[crypt])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
if test "$GCC" = "yes"; then
#  CFLAGS="$CFLAGS -Wall -Wunused -Wredundant-decls -funsigned-char"
  CFLAGS="$CFLAGS -Wall -Wunused -funsigned-char -Wno-pointer-sign"
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
AM_GNU_GETTEXT_VERSION(0.16.1)

dnl ***************************************************************************
dnl Find pkg-config
dnl ***************************************************************************
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://pkgconfig.sourceforge.net])
fi
dnl ***************************************************************************
dnl checking libmagic
dnl ***************************************************************************
have_libmagic=false
AC_CHECK_HEADERS(magic.h,[AC_CHECK_LIB(magic, magic_open, have_libmagic=true)])
if ! ${have_libmagic}; then
   AC_MSG_WARN([libmagic in 'file' is required. if you use Debian GNU/Linux, libmagic is contained 'libmagic-dev' package.])
  MAGIC_LIBS=""
else
  MAGIC_LIBS="-lmagic"
  AC_DEFINE(HAVE_LIBMAGIC,1,"")
fi
AC_SUBST(MAGIC_LIBS)

dnl ***************************************************************************
dnl glclient Support
dnl ***************************************************************************

AC_ARG_ENABLE(client_gtk1,
  [  --enable-client-gtk1 use gtk1 [[disable]]],,[enable_client_gtk1="no"])
if test "x$enable_client_gtk1" = "xyes"; then
	PKG_CHECK_MODULES(GTK1CLIENT,
	[
		libglade-panda-gnome >= 0.16
		gtkpanda             >= 1.0.0
		libxml               >= 1.7.2
		libmondai            >= 1.4.0
	])
fi
AM_CONDITIONAL(GTK1CLIENT, test "x$enable_client_gtk1" = "xyes")

AC_ARG_ENABLE(client_gtk2,
  [  --enable-client-gtk2 use gtk2 [[disable]]],,[enable_client_gtk2="no"])
if test "x$enable_client_gtk2" = "xyes"; then
	PKG_CHECK_MODULES(GTK2CLIENT,
	[
		libglade-panda-2.0	>= 1.9.0
		libmondai			>= 1.4.0
	])
	AC_CHECK_LIB(gtkpanda-2.0, gtk_panda_pdf_new,
	    enable_pdf=1,
	    enable_pdf=0)
	if test $enable_pdf = 1 ; then
	    AC_DEFINE(USE_PDF, 1, "")
	fi
fi
AM_CONDITIONAL(GTK2CLIENT, test "x$enable_client_gtk2" = "xyes")

if test "x$enable_client_gtk1" = "xyes" -o "x$enable_client_gtk2" = "xyes"
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
if test $withval != yes 
then 
  dir=$withval 
else 
  dir="/usr/local/" 
fi 
AC_MSG_CHECKING(for Postgres files) 
postgresdir="" 
for d in $dir /usr /usr/local/postgres /opt/postgres /opt/packages/postgres /disk/postgres /usr/local/pgsql /sw
do 
  if test -f $d/lib/libpq.$SO_SUFFIX
  then 
      AC_MSG_RESULT(found Postgres in $d) 
      postgresdir=$d 
      break 
  fi 
done 
if test x$postgresdir = x 
then 
  AC_MSG_WARN(Postgres backend not used) 
else 
  if test -f ${postgresdir}/include/libpq-fe.h 
  then 
      POSTGRES_CFLAGS=-I${postgresdir}/include 
  elif test -f ${postgresdir}/include/pgsql/libpq-fe.h 
  then 
      POSTGRES_CFLAGS=-I${postgresdir}/include/pgsql 
  elif test -f ${postgresdir}/include/postgresql/libpq-fe.h 
  then 
      POSTGRES_CFLAGS=-I${postgresdir}/include/postgresql 
  elif test -f ${postgresdir}/include/postgresql/8.0/libpq-fe.h 
  then 
      POSTGRES_CFLAGS=-I${postgresdir}/include/postgresql/8.0
  else 
      AC_MSG_WARN(Postgres include files not found, backend not used) 
      postgresdir="" 
  fi 
  POSTGRES_LIBS="-L${postgresdir}/lib -lpq" 
  AC_DEFINE(HAVE_POSTGRES,1,"") 
  enable_postgres="yes"
fi 
 
#AM_CONDITIONAL(POSTGRES, test x$postgresdir != x) 
AC_SUBST(POSTGRES_LIBS) 
AC_SUBST(POSTGRES_CFLAGS) 
])

dnl *************************************************************************** 
dnl Add all the cflags & libs needed for the Oseki
dnl *************************************************************************** 
enable_oseki="no"
AC_PATH_PROG(OSEKI_CONFIG,oseki-config,no)
AC_ARG_WITH(oseki, 
	[  --with-oseki use Oseki/Search backend in <directory>],[
	if test x$withval = xyes; then
        enable_oseki="yes"
		AC_MSG_CHECKING(for Oseki files) 
		OSEKI_LIBS=`oseki-config --libs`
		OSEKI_CFLAGS=`oseki-config --cflags`
	fi 
	#AM_CONDITIONAL(OSEKI, test x$osekidir != x) 
    AC_DEFINE(HAVE_OSEKI,1,"") 
	AC_SUBST(OSEKI_LIBS) 
	AC_SUBST(OSEKI_CFLAGS) 
])

dnl ***************************************************************************
dnl shell driver
dnl ***************************************************************************

AC_ARG_ENABLE(shell,
    [  --disable-shell         disable shell driver [[enable]]],,[enable_shell="yes"])
if test "x$enable_shell" = "xyes"; then
   AC_DEFINE(USE_SHELL,1,"")
fi

dnl *************************************************************************** 
dnl for the dotCOBOL
dnl *************************************************************************** 
 
enable_dotcobol="no"
AC_ARG_WITH(dotcobol, 
[  --with-dotcobol=<directory> use dotCOBOL in <directory>],[
	if test $withval != yes 
	then 
	  dir=$withval 
	else 
	  dir="/usr/local" 
	fi 
	AC_MSG_CHECKING(for dotCOBOL files) 
	dotcoboldir="" 
	for d in $dir /usr /usr/local /usr/local/dotcobol /opt/dotcobol /opt/packages/dotcobol /disk/dotcobol
	do 
	  if test -f $d/bin/fcu
	  then 
	      AC_MSG_RESULT(found dotCOBOL in $d) 
	      dotcoboldir=$d 
	      break 
	  fi 
	done 
	if test x$dotcoboldir = x 
	then 
	  AC_MSG_WARN(dotCOBOL not used) 
	else 
	  DOTCOBOL_COMMAND="${dotcoboldir}/bin/fcu"
	  AC_DEFINE(HAVE_DOTCOBOL,,"")
          enable_dotcobol="yes"
	fi 
 
#	AM_CONDITIONAL(DOTCOBOL, test x$dotcoboldir != x) 
	AC_SUBST(DOTCOBOL_COMMAND) 
]) 

dnl *************************************************************************** 
dnl for the OpenCOBOL23
dnl *************************************************************************** 
AC_ARG_ENABLE(opencobol23,
  [  --enable-opencobol23      enable OpenCOBOL23 [[disable]]],,[enable_opencobol23="no"])
if test "x$enable_opencobol23" = "xyes"; then
    AC_MSG_CHECKING(for OpenCOBOL23)
    cob23_exists=yes
    cob-config23 --version > /dev/null 2> /dev/null || cob_exists=no
    if test "x$cob23_exists" = "xyes"; then
        COB_CFLAGS="`cob-config23 --cflags`"
        COB_LIBS="`cob-config23 --libs`"
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_OPENCOBOL23,1,"")
        AC_DEFINE(HAVE_OPENCOBOL,1,"")
    else
        AC_MSG_RESULT(no)
        enable_opencobol23="no"
    fi
    AC_SUBST(COB23_CFLAGS)
    AC_SUBST(COB23_LIBS)
    
    OPENCOBOL23_CFLAGS="$COB_CFLAGS"
    OPENCOBOL23_LIBS="$COB_LIBS"
    
    AC_SUBST(OPENCOBOL23_LIBS) 
    AC_SUBST(OPENCOBOL23_CFLAGS) 
fi
AM_CONDITIONAL(OPENCOBOL23, test "x$enable_opencobol23" = "xyes") 

dnl *************************************************************************** 
dnl for the OpenCOBOL08
dnl *************************************************************************** 
AC_ARG_ENABLE(opencobol08,
  [  --enable-opencobol08      enable OpenCOBOL08 [[disable]]],,[enable_opencobol08="no"])
if test "x$enable_opencobol08" = "xyes"; then
    AC_MSG_CHECKING(for OpenCOBOL08)
    cob08_exists=yes
    cob-config08 --version > /dev/null 2> /dev/null || cob_exists=no
    if test "x$cob08_exists" = "xyes"; then
        COB_CFLAGS="`cob-config08 --cflags`"
        COB_LIBS="`cob-config08 --libs`"
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_OPENCOBOL08,1,"")
        AC_DEFINE(HAVE_OPENCOBOL,1,"")
    else
        AC_MSG_RESULT(no)
        enable_opencobol08="no"
    fi
    AC_SUBST(COB08_CFLAGS)
    AC_SUBST(COB08_LIBS)
    
    OPENCOBOL08_CFLAGS="$COB_CFLAGS"
    OPENCOBOL08_LIBS="$COB_LIBS"
    
    AC_SUBST(OPENCOBOL08_LIBS) 
    AC_SUBST(OPENCOBOL08_CFLAGS) 
fi
AM_CONDITIONAL(OPENCOBOL08, test "x$enable_opencobol08" = "xyes") 

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
dnl for Ruby
dnl *************************************************************************** 

enable_ruby="no"
AC_ARG_WITH(ruby, 
[  --with-ruby=PATH        path to ruby],[
    if test "x$withval" = "xyes"; then
      AC_PATH_PROG(RUBY, ruby, no)
    else
      AC_MSG_CHECKING(for ruby)
      RUBY="$withval"
      AC_MSG_RESULT($RUBY)
    fi
    if test "x$RUBY" != "xno"; then
      AC_MSG_CHECKING(for ruby version >= 1.8.0)
      if $RUBY -e 'exit(RUBY_VERSION >= "1.8.0" ? 0 : 1)'; then
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR(Ruby version is too old)
      fi
      archdir=`$RUBY -r rbconfig -e 'print Config::CONFIG[["archdir"]]'`
      if test -f $archdir/ruby.h; then
        AC_DEFINE(HAVE_RUBY,1,"")
        enable_ruby="yes"
        RUBY_CFLAGS="-I${archdir}"
        RUBY_SHARED=`$RUBY -r rbconfig -e 'print Config::CONFIG[["ENABLE_SHARED"]]'`
        if test "$RUBY_SHARED" = "yes"; then
          RUBY_LIBS=`$RUBY -r rbconfig -e 'print "-L" + Config::CONFIG[["libdir"]] + " " + Config::CONFIG[["LIBRUBYARG"]].gsub(/-L\./, "-L" + Config::CONFIG[["libdir"]]) + " " + Config::CONFIG[["LIBS"]]'`
          RUBY_EXT_LIBS="$RUBY_LIBS"
        else
          RUBY_LIBS=`$RUBY -r rbconfig -e 'print Config::CONFIG[["libdir"]] + "/" + Config::CONFIG[["LIBRUBY_A"]] + " " + Config::CONFIG[["LIBS"]]'`
          RUBY_EXT_LIBS=`$RUBY -r rbconfig -e 'Config::CONFIG[["LIBS"]]'`
        fi
        rubylibdir=`$RUBY -r rbconfig -e 'print Config::CONFIG[["rubylibdir"]]'`
        rubyarchdir=`$RUBY -r rbconfig -e 'print Config::CONFIG[["archdir"]]'`
      else
        AC_MSG_ERROR(ruby.h not found)
      fi
    fi
    AC_SUBST(RUBY) 
    AC_SUBST(RUBY_CFLAGS) 
    AC_SUBST(RUBY_SHARED)
    AC_SUBST(RUBY_LIBS)
    AC_SUBST(RUBY_EXT_LIBS)
    AC_SUBST(rubylibdir) 
    AC_SUBST(rubyarchdir) 
],[
    AC_MSG_CHECKING(for ruby)
    AC_MSG_RESULT(no)
])

dnl ***************************************************************************
dnl pty message display
dnl ***************************************************************************

AC_ARG_ENABLE(pty,
  [  --enable-pty            enable pty message [[disable]]],,[enable_pty="no"])
if test "x$enable_pty" = "xyes"; then
    AC_DEFINE(USE_PTY,1,"")
fi

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
		AC_MSG_CHECKING(for OpenSSL files) 
		ssldir="" 
		for d in $dir /usr /usr/local/ssl /opt/ssl /opt/packages/ssl /disk/ssl
		do 
		  if test -f $d/lib/libssl.$SO_SUFFIX
		  then 
		      AC_MSG_RESULT(found OpenSSL lib in $d) 
		      ssldir=$d 
		      break 
		  fi 
		done 
		if test x$ssldir = x 
		then 
			AC_MSG_WARN(SSL not used) 
		else 
			if test -f $ssldir/include/openssl/ssl.h
			then 
			   	AC_MSG_RESULT(found OpenSSL include in $d) 
				SSL_CFLAGS="-I${ssldir}/include"
				SSL_LIBS="-L${ssldir}/lib -lssl" 
				AC_DEFINE(USE_SSL,1,"")
				enable_ssl="yes"
			else 
				AC_MSG_WARN(SSL not used) 
			fi
		fi
	fi 
#	AM_CONDITIONAL(SSL, test x$ssldir != x) 
	AC_SUBST(SSL_LIBS) 
	AC_SUBST(SSL_CFLAGS) 
])		

dnl ***************************************************************************
dnl PKCS11
dnl ***************************************************************************
enable_pkcs11="no"
AC_ARG_WITH(pkcs11,
	[  --with-pkcs11=<directory>  use engine_pkcs11.$SO_SUFFIX in <directory>],[
	if test x$withval = xyes
	then
		engine_pkcs11="/usr/lib/opensc/engine_pkcs11.$SO_SUFFIX" 
	else
		engine_pkcs11=$withval 
 	fi
	pkcs11_header=""
	for d in $dir /usr/ /usr/lib/ /opt
	do 
		if test -f $d/include/opensc/rsaref/pkcs11.h 
		then 
			AC_MSG_RESULT(found pkcs11.h in $d) 
			pkcs11_header=$d/include/opensc/rsaref/pkcs11.h  
			break 
		fi 
	done 
	if test x$pkcs11_header = x 
	then
		AC_MSG_WARN(pkcs11 header not used) 
	else
		AC_DEFINE(USE_PKCS11,1,"")
		AC_DEFINE_UNQUOTED(ENGINE_PKCS11_PATH,"$engine_pkcs11","")
		enable_pkcs11="yes"
	fi
])

dnl ***************************************************************************
dnl eRuby
dnl ***************************************************************************

enable_eruby="no"
AC_ARG_WITH(eruby,
	[  --with-eruby=<path>  use ERuby in <path>],[
    if test "x$withval" = "xyes"; then
      ERUBY_PATH="/usr/bin/eruby";
    else
      AC_MSG_CHECKING(for ruby)
      ERUBY_PATH="$withval"
    fi
    enable_eruby="yes"
    AC_SUBST(ERUBY_PATH)
    AC_DEFINE_UNQUOTED(ERUBY_PATH,"$ERUBY_PATH","")
    AC_DEFINE(USE_ERUBY,1,"")
])		

dnl ***************************************************************************
dnl dorobune wo oki de shizumeru
dnl ***************************************************************************

AC_ARG_ENABLE(oki,
  [  --enable-oki            DOROBUNE WO OKI DE SHIZUMERU [[disable]]],,[enable_oki="no"])
if test "x$enable_oki" = "xyes"; then
    AC_DEFINE(OKI,1,"")
fi

dnl ***************************************************************************
dnl tinyMCE support
dnl ***************************************************************************

AC_ARG_ENABLE(mce,
  [  --enable-mce            MCE input  [[disable]]],,[enable_mce="no"])
if test "x$enable_mce" = "xyes"; then
    AC_DEFINE(USE_MCE,1,"")
fi

dnl ***************************************************************************
dnl IE5.x support
dnl ***************************************************************************

AC_ARG_ENABLE(ie5,
  [  --enable-ie5            IE 5.x compatibility [[disable]]],,[enable_ie5="no"])
if test "x$enable_ie5" = "xyes"; then
    AC_DEFINE(USE_IE5,1,"")
fi

dnl ***************************************************************************
dnl use msgd for message output
dnl ***************************************************************************

AC_ARG_ENABLE(msgd,
  [  --enable-msgd           enable msgd [[disable]]],,[enable_msgd="no"])
if test "x$enable_msgd" = "xyes"; then
    AC_DEFINE(USE_MSGD,1,"")
fi

dnl ***************************************************************************
dnl use syslog for message output
dnl ***************************************************************************

AC_ARG_ENABLE(syslog,
  [  --enable-syslog           enable logging to syslog [[disable]]],,[enable_syslog="no"])
if test "x$enable_syslog" = "xyes"; then
    AC_DEFINE(USE_SYSLOG,1,"")
fi

dnl ***************************************************************************
dnl Add all the cflags & libs needed for the glib modules.
dnl ***************************************************************************

AC_PATH_PROG(GLIB_CONFIG,glib-config,no)
if test x$GLIB_CONFIG = xno; then
  AC_MSG_ERROR(Couldn't find glib-config)
fi
GLIB_LIBS=`glib-config --libs`
GLIB_CFLAGS=`glib-config --cflags`
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

dnl *************************************************************************** 
dnl Add all the cflags & libs needed for the GD modules. 
dnl *************************************************************************** 
 
AC_PATH_PROG(GD_CONFIG,gdlib-config,no) 
if test x$GD_CONFIG = xno; then 
  AC_MSG_WARN(Couldn't find gdlib-config) 
  GDLIB_LDFLAGS=
  GDLIB_CFLAGS=
 else 
  GDLIB_LDFLAGS=`gdlib-config --ldflags`' -lgd'
  GDLIB_CFLAGS=`gdlib-config --cflags` 
  AC_DEFINE(HAVE_GD,1,"") 
fi 
AC_SUBST(GDLIB_LDFLAGS) 
AC_SUBST(GDLIB_CFLAGS) 
 
dnl ***************************************************************************
dnl Add all the cflags & libs needed for the XML2 modules.
dnl ***************************************************************************

AC_PATH_PROG(XML2_CONFIG,xml2-config,no)
if test x$XML2_CONFIG = xno; then
  AC_MSG_ERROR(Couldn't find xml2-config)
fi
XML2_LIBS=`xml2-config --libs`
XML2_CFLAGS=`xml2-config --cflags`
AC_SUBST(XML2_LIBS)
AC_SUBST(XML2_CFLAGS)

AC_PATH_PROG(XSLT_CONFIG,xslt-config,no)
if test x$XSLT_CONFIG = xno; then
  AC_MSG_ERROR(Couldn't find xslt-config)
fi
XSLT_LIBS=`xslt-config --libs`' -lexslt'
XSLT_CFLAGS=`xslt-config --cflags`' -I /usr/include/libexslt'
AC_SUBST(XSLT_LIBS)
AC_SUBST(XSLT_CFLAGS)

dnl ***************************************************************************
dnl Add all the cflags & libs needed for the libmondai modules.
dnl ***************************************************************************
PKG_CHECK_MODULES(MONDAI, libmondai >= 1.4.0 )
AC_SUBST(MONDAI_LIBS)
AC_SUBST(MONDAI_CFLAGS)

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
   MONTSUQI_LIBRARY_PATH=$libdir
fi
AC_SUBST(MONTSUQI_LIBRARY_PATH)

AC_ARG_WITH(mon-loadpath,
	[  --with-mon-loadpath=<DIR>],
	[mon_loadpath=$withval])
if test "$mon_loadpath" != ""; then
   MONTSUQI_LOAD_PATH=$mon_loadpath
else
   MONTSUQI_LOAD_PATH=$libdir
fi
AC_SUBST(MONTSUQI_LOAD_PATH)

AC_CONFIG_FILES([Makefile 
                 intl/Makefile 
                 po/Makefile.in
                 Doc/Makefile
                 aps/Makefile
                 glauth/Makefile
                 cobol/Makefile
                 dblib/Makefile
                 dbstuff/Makefile
                 fdd/Makefile
                 glclient/Makefile
                 glclient/gtk1/Makefile
                 glclient/gtk2/Makefile
                 glserver/Makefile
                 htserver/Makefile
                 include/Makefile
                 libs/Makefile
                 monpls_demo/Makefile
                 pgserver/Makefile
                 tools/Makefile
                 wfc/Makefile
                 panda-config
])
#####], [chmod +x panda-config])
AC_OUTPUT

dnl ***************************************************************************
dnl output result
dnl ***************************************************************************

echo "------------------------------------------------------"
echo "$PACKAGE Configuration:"
echo ""
echo "  VERSION:               $VERSION"
echo "  Gtk1 client support:   $enable_client_gtk1"
echo "  Gtk2 client support:   $enable_client_gtk2"
echo "  PostgreSQL support:    $enable_postgres"
echo "  shell driver support:  $enable_shell"
echo "  Oseki/Search support:  $enable_oseki"
echo "  dotCOBOL support:      $enable_dotcobol"
echo "  OpenCOBOL23 support:   $enable_opencobol23"
echo "  OpenCOBOL08 support:   $enable_opencobol08"
echo "  OpenCOBOL support:     $enable_opencobol"
echo "  Ruby support:          $enable_ruby"
echo "  pty message support:   $enable_pty"
echo "  IP v6 support:         $enable_ipv6"
echo "  SSL support:           $enable_ssl"
echo "  PKCS11 support:        $enable_pkcs11"
echo "  OKI support:           $enable_oki"
echo "  msgd support:          $enable_msgd"
echo "  syslog support:        $enable_syslog"
echo "  IE5 support:           $enable_ie5"
echo "  MCE support:           $enable_mce"
echo "  ERUBY support:         $enable_eruby"
echo ""
