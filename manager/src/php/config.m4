dnl $Id$
dnl config.m4 for extension qconf_manager

PHP_ARG_WITH(qconfzk-dir,  for libqconfzk,
[  --with-qconfzk-dir[=DIR]   Set the path to libqconfzk include prefix.], yes)

PHP_ARG_ENABLE(qconf_manager, whether to enable qconf_manager support,
[  --enable-qconf_manager           Enable qconf_manager support])


if test "$PHP_QCONF_MANAGER" != "no"; then
    if test "$PHP_QCONFZK_DIR" != "no" && test "$PHP_QCONFZK_DIR" != "yes"; then
        if test -r "$PHP_QCONFZK_DIR/qconf_zk.h"; then
            PHP_QCONFZK_INCDIR="$PHP_QCONFZK_DIR"
            PHP_ZOOKEEPER_INCDIR="$PHP_QCONFZK_DIR/zookeeper"
        else
            AC_MSG_ERROR([Can't find qconf headers under "$PHP_PHP_QCONFZK_DIR"])
        fi
    else
        AC_MSG_ERROR([Need arg --with-libqconfzk-dir"])
    fi

    PHP_ADD_INCLUDE($PHP_QCONFZK_INCDIR)
    PHP_ADD_INCLUDE($PHP_ZOOKEEPER_INCDIR)
    PHP_REQUIRE_CXX()  

    PHP_ADD_LIBRARY(stdc++, 1, EXTRA_LDFLAGS)

    PHP_NEW_EXTENSION(qconf_manager, qconf_manager.cc, $ext_shared)
fi
