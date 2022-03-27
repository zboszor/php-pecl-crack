/*
   +----------------------------------------------------------------------+
   | PHP Version 7+                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2005 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Alexander Feldman                                           |
   |          Sascha Kettler                                              |
   +----------------------------------------------------------------------+
 */
/* $Id: crack.c,v 1.26 2005/09/21 08:59:57 skettler Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_globals.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#if PHP_MAJOR_VERSION >= 8
#define TSRMLS_DC
#define TSRMLS_CC
#endif

#if HAVE_CRACK

#include "php_crack.h"
#include <packer.h>
#include <sys/stat.h>

/* True global resources - no need for thread safety here */
static int le_crack;

/* {{{ crack_functions[]
 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_crack_opendict, 0, 0, 1)
	ZEND_ARG_VARIADIC_INFO(0, dictionary)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_crack_closedict, 0, 0, 1)
	ZEND_ARG_INFO(0, dictionary)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_crack_check, 0, 0, 4)
	ZEND_ARG_INFO(0, dictionary)
	ZEND_ARG_INFO(0, password)
	ZEND_ARG_VARIADIC_INFO(0, username)
	ZEND_ARG_VARIADIC_INFO(0, gecos)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_crack_getlastmessage, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_crack_fascistcheck, 0, 0, 2)
	ZEND_ARG_INFO(0, password)
	ZEND_ARG_VARIADIC_INFO(0, dictionary)
ZEND_END_ARG_INFO()

static zend_function_entry crack_functions[] = {
	PHP_FE(crack_opendict, arginfo_crack_opendict)
	PHP_FE(crack_closedict, arginfo_crack_closedict)
	PHP_FE(crack_check,	arginfo_crack_check)
	PHP_FE(crack_getlastmessage, arginfo_crack_getlastmessage)
	PHP_FE(crack_fascistcheck, arginfo_crack_fascistcheck)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ crack_module_entry
 */
zend_module_entry crack_module_entry = {
	STANDARD_MODULE_HEADER,
	"crack",
	crack_functions,
	PHP_MINIT(crack), 
	PHP_MSHUTDOWN(crack),
	PHP_RINIT(crack),
	PHP_RSHUTDOWN(crack),
	PHP_MINFO(crack),
	"1.0",
	STANDARD_MODULE_PROPERTIES,
};
/* }}} */

ZEND_DECLARE_MODULE_GLOBALS(crack)

#ifdef COMPILE_DL_CRACK
ZEND_GET_MODULE(crack)
#endif

/* {{{ PHP_INI */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("crack.default_dictionary", NULL, PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateString, default_dictionary, zend_crack_globals, crack_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_crack_init_globals
 */
static void php_crack_init_globals(zend_crack_globals *crack_globals)
{
	crack_globals->last_message = NULL;
}
/* }}} */

/* {{{ php_crack_checkpath
 */
static int php_crack_checkpath(char *path TSRMLS_DC, int path_len TSRMLS_DC)
{
	char *filename;
	int filename_len;
	int result = SUCCESS;
	struct stat st;

	if (!path || !path_len)
		return FAILURE;

	filename_len = path_len + 10;
	filename = (char *) emalloc(filename_len);
	if (!filename)
		return FAILURE;

	memcpy(filename, path, path_len);

	filename[path_len] = '\0';
	strcat(filename, ".pwd");
	if (stat(filename, &st) != 0)
		return FAILURE;

	filename[path_len] = '\0';
	strcat(filename, ".pwi");
	if (stat(filename, &st) != 0)
		return FAILURE;

	filename[path_len] = '\0';
	strcat(filename, ".hwm");
	if (stat(filename, &st) != 0)
		return FAILURE;

	filename[path_len] = '\0';
	if (php_check_open_basedir(filename TSRMLS_CC))
		return FAILURE;

	return SUCCESS;
}
/* }}} */

/* {{{ php_crack_module_dtor
 */
static void php_crack_module_dtor(zend_resource *rsrc TSRMLS_DC)
{
	PWDICT *pwdict = (PWDICT *) rsrc->ptr;

	if (pwdict != NULL) {
		PWClose(pwdict);
	}
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(crack)
{
#ifdef ZTS
	ZEND_INIT_MODULE_GLOBALS(crack, php_crack_init_globals, NULL);
#endif

	REGISTER_INI_ENTRIES();
	le_crack = zend_register_list_destructors_ex(php_crack_module_dtor, NULL, "crack dictionary", module_number);

	return SUCCESS;
}

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(crack)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(crack)
{
	CRACKG(last_message) = NULL;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
ZEND_MODULE_DEACTIVATE_D(crack)
{
	if (NULL != CRACKG(last_message)) {
		efree(CRACKG(last_message));
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(crack)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "crack support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ proto resource crack_opendict([string dictionary])
   Opens a new cracklib dictionary */
PHP_FUNCTION(crack_opendict)
{
	char *path = NULL;
	size_t path_len;
	PWDICT *pwdict;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &path, &path_len) == FAILURE)
		RETURN_FALSE;

	if (!path || !path_len) {
		path = CRACKG(default_dictionary);
		path_len = (path ? strlen(path) : 0);
	}

	if (php_crack_checkpath(path TSRMLS_CC, path_len TSRMLS_CC) == FAILURE)
		RETURN_FALSE;

	pwdict = PWOpen(path, "r");

	if (NULL == pwdict) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not open crack dictionary: %s", path);
		RETURN_FALSE;
	}

	RETURN_RES(zend_register_resource(pwdict, le_crack));
}
/* }}} */

/* {{{ proto bool crack_closedict(resource dictionary)
   Closes an open cracklib dictionary */
PHP_FUNCTION(crack_closedict)
{
	zval *dictionary = NULL;
	PWDICT *pwdict;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &dictionary)) {
		RETURN_FALSE;
	}

	pwdict = (PWDICT *)zend_fetch_resource(Z_RES_P(dictionary), "crack dictionary", le_crack);
	if (pwdict == NULL)
		RETURN_FALSE;

	zend_list_delete(Z_RES_P(dictionary));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool crack_check(resource dictionary, string password [, string username [, string gecos]])
   Performs an obscure check with the given password */
PHP_FUNCTION(crack_check)
{
	zval *dictionary = NULL;
	char *password = NULL;
	int password_len;
	char *username = NULL;
	int username_len;
	char *gecos = NULL;
	int gecos_len;
	char *message;
	PWDICT *pwdict;

	if (NULL != CRACKG(last_message)) {
		efree(CRACKG(last_message));
		CRACKG(last_message) = NULL;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|ss", &dictionary, &password, &password_len, &username, &username_len, &gecos, &gecos_len) == FAILURE)
		RETURN_FALSE;

	pwdict = (PWDICT *)zend_fetch_resource(Z_RES_P(dictionary), "crack dictionary", le_crack);
	if (pwdict == NULL)
		RETURN_FALSE;

	message = FascistLookUser(pwdict, password, username, gecos);

	if (NULL == message) {
		CRACKG(last_message) = estrdup("strong password");
		RETURN_TRUE;
	}
	else {
		CRACKG(last_message) = estrdup(message);
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string crack_getlastmessage(void)
   Returns the message from the last obscure check */
PHP_FUNCTION(crack_getlastmessage)
{
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (NULL == CRACKG(last_message)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No obscure checks in this session");
		RETURN_FALSE;
	}

	RETURN_STRING(CRACKG(last_message));
}
/* }}} */

/* {{{ proto bool|string crack_fascistcheck(string password, [string dictionary])
   Calls FascistCheck from cracklib */
PHP_FUNCTION(crack_fascistcheck)
{
	char *password = NULL;
	size_t password_len;
	char *path = NULL;
	size_t path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &password, &password_len, &path, &path_len) == FAILURE)
		RETURN_FALSE;

	if (!path || !path_len) {
		path = CRACKG(default_dictionary);
		path_len = (path ? strlen(path) : 0);
	}

	if (php_crack_checkpath(path TSRMLS_CC, path_len TSRMLS_CC) == FAILURE)
		RETURN_FALSE;

	const char *result = FascistCheck(password, path);

	if (result == NULL)
		RETURN_TRUE;

	RETURN_STRING(estrdup(result));
}
/* }}} */

#endif /* HAVE_CRACK */
