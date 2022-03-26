dnl
dnl $Id: config.m4,v 1.11 2005/09/05 12:27:12 skettler Exp $
dnl

PHP_ARG_WITH(crack, for cracklib support,
[  --with-crack            Include crack support.])

if test "$PHP_CRACK" != "no"; then

    for i in $PHP_CRACK/lib $PHP_CRACK/cracklib /usr/local/lib /usr/lib; do
      test -f $i/libcrack.$SHLIB_SUFFIX_NAME -o -f $i/libcrack.a && CRACK_LIBDIR=$i && break
    done

    if test -z "$CRACK_LIBDIR"; then
      AC_MSG_ERROR(Cannot find the cracklib library file)
    fi

    for i in $PHP_CRACK/include $PHP_CRACK/cracklib /usr/local/include /usr/include; do
      test -f $i/packer.h && CRACK_INCLUDEDIR=$i && break
    done
  
    if test -z "$CRACK_INCLUDEDIR"; then
      AC_MSG_ERROR(Cannot find a cracklib header file)
    fi

    PHP_ADD_INCLUDE($CRACK_INCLUDEDIR)
    PHP_ADD_LIBRARY_WITH_PATH(crack, $CRACK_LIBDIR, CRACK_SHARED_LIBADD)

    PHP_NEW_EXTENSION(crack, crack.c, $ext_shared)
    PHP_SUBST(CRACK_SHARED_LIBADD)
    AC_DEFINE(HAVE_CRACK, 1, [ Define to 1 if we are building with the crack library. ])
    AC_DEFINE(HAVE_CRACK_BUNDLED, 0, [ Define to 1 to build against the bundled crack library. ])

fi
