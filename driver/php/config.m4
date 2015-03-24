dnl
dnl $ Id: $
dnl vim:se ts=2 sw=2 et:

PHP_ARG_ENABLE(qconf, whether to enable qconf support,
[  --enable-qconf               Enable qconf support])

dnl PHP_ARG_ENABLE(qconf-compatible, whether to enable qconf compatible support,
dnl [  --enable-qconf-compatible               Enable qconf compatible  support])

PHP_ARG_WITH(libqconf-dir,  for libqconf,
[  --with-libqconf-dir[=DIR]   Set the path to libqconf include prefix.], yes)

if test -z "$PHP_DEBUG"; then
  AC_ARG_ENABLE(debug,
  [  --enable-debug          compile with debugging symbols],[
    PHP_DEBUG=$enableval
  ],[    PHP_DEBUG=no
  ])
fi

if test "$PHP_QCONF" != "no"; then
 
   if test "$PHP_LIBQCONF_DIR" != "no" && test "$PHP_LIBQCONF_DIR" != "yes"; then
     if test -r "$PHP_LIBQCONF_DIR/qconf.h"; then
       PHP_LIBQCONF_INCDIR="$PHP_LIBQCONF_DIR"
     else
       AC_MSG_ERROR([Can't find qconf headers under "$PHP_LIBQCONF_DIR"])
     fi
   else
     PHP_LIBQCONF_DIR="no"
     for i in /usr/local/include/qconf; do
       if test -r "$i/qconf.h"; then
         PHP_LIBQCONF_INCDIR=$i
     break
       fi
     done

     if test "$PHP_LIBQCONF_INCDIR" = ""; then
       AC_MSG_ERROR([Can't find qconf headers under "$PHP_LIBQCONF_DIR"])
     fi
   fi

dnl    PHP_LIBQCONF_INCDIR="/usr/local/include/qconf"
    PHP_ADD_INCLUDE($PHP_LIBQCONF_INCDIR)

    PHP_REQUIRE_CXX()
    PHP_ADD_LIBRARY(stdc++, "", EXTRA_LDFLAGS)
    PHP_NEW_EXTENSION(qconf, php_qconf.c $SESSION_EXTRA_FILES, $ext_shared)

dnl  fi

fi
