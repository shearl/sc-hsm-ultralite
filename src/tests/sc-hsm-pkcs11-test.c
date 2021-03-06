/**
 * SmartCard-HSM PKCS#11 Module
 *
 * Copyright (c) 2013, CardContact Systems GmbH, Minden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of CardContact Systems GmbH nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CardContact Systems GmbH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file testpkcs11.c
 * @author Andreas Schwier
 * @brief Unit test for PKCS#11 interface
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <common/mutex.h>

#define MUTEX_LOCK(pmutex) assert(!mutex_lock(pmutex))
#define MUTEX_UNLOCK(pmutex) assert(!mutex_unlock(pmutex))

/* Number of threads used for multi-threading test */
/* Default PIN unless --pin is defined */
#define PIN "123456"


#ifndef _WIN32
#include <dlfcn.h>
#include <semaphore.h>
#include <pthread.h>

#define LIB_HANDLE void*
#define P11LIBNAME "libsc-hsm-pkcs11.so"

typedef sem_t SEMAPHORE;

/* Windows GetTickCount() returns ms since startup.
 * This function returns ms since the Epoch.
 * Since we're doing a delta it's OK
 */
long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#else /* _WIN32 */

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <windows.h>
#include <malloc.h>

#define LIB_HANDLE HMODULE
#define P11LIBNAME "sc-hsm-pkcs11.dll"

#define usleep(us) Sleep((us) / 1000)

#define dlopen(fn, flag) LoadLibrary(fn)
#define dlclose(h) FreeLibrary(h)
#define dlsym(h, n) GetProcAddress(h, n)

#define pthread_t HANDLE
#define pthread_create(t, a, f, p) (*t = CreateThread(0, 0, f, p, 0, 0), *t ? 0 : GetLastError())
#define pthread_detach(t) (CloseHandle(t) ? 0 : GetLastError())

typedef HANDLE SEMAPHORE;
int sem_init(SEMAPHORE *psem, int pshared, unsigned count)
{
	*psem = CreateSemaphore(0, count, count, 0);
	if (*psem == 0)
		return GetLastError();
	return 0;
}

#define sem_wait(psem) (WaitForSingleObject(*psem, INFINITE), 0)
#define sem_post(psem) ReleaseSemaphore(*psem, 1, 0)
#define sem_destroy(psem) (CloseHandle(*psem) ? 0 : GetLastError())

char* dlerror()
{
	char* msg = "UNKNOWN";
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, (char*)&msg, 0, 0);
	return msg;
}

#endif /* _WIN32 */



#include <pkcs11/cryptoki.h>

struct id2name_t {
	unsigned long       id;
	char                *name;
	unsigned long       attr;
};

struct id2name_t p11CKRName[] = {
		{ CKR_CANCEL                            , "CKR_CANCEL", 0 },
		{ CKR_HOST_MEMORY                       , "CKR_HOST_MEMORY", 0 },
		{ CKR_SLOT_ID_INVALID                   , "CKR_SLOT_ID_INVALID", 0 },
		{ CKR_GENERAL_ERROR                     , "CKR_GENERAL_ERROR", 0 },
		{ CKR_FUNCTION_FAILED                   , "CKR_FUNCTION_FAILED", 0 },
		{ CKR_ARGUMENTS_BAD                     , "CKR_ARGUMENTS_BAD", 0 },
		{ CKR_NO_EVENT                          , "CKR_NO_EVENT", 0 },
		{ CKR_NEED_TO_CREATE_THREADS            , "CKR_NEED_TO_CREATE_THREADS", 0 },
		{ CKR_CANT_LOCK                         , "CKR_CANT_LOCK", 0 },
		{ CKR_ATTRIBUTE_READ_ONLY               , "CKR_ATTRIBUTE_READ_ONLY", 0 },
		{ CKR_ATTRIBUTE_SENSITIVE               , "CKR_ATTRIBUTE_SENSITIVE", 0 },
		{ CKR_ATTRIBUTE_TYPE_INVALID            , "CKR_ATTRIBUTE_TYPE_INVALID", 0 },
		{ CKR_ATTRIBUTE_VALUE_INVALID           , "CKR_ATTRIBUTE_VALUE_INVALID", 0 },
		{ CKR_DATA_INVALID                      , "CKR_DATA_INVALID", 0 },
		{ CKR_DATA_LEN_RANGE                    , "CKR_DATA_LEN_RANGE", 0 },
		{ CKR_DEVICE_ERROR                      , "CKR_DEVICE_ERROR", 0 },
		{ CKR_DEVICE_MEMORY                     , "CKR_DEVICE_MEMORY", 0 },
		{ CKR_DEVICE_REMOVED                    , "CKR_DEVICE_REMOVED", 0 },
		{ CKR_ENCRYPTED_DATA_INVALID            , "CKR_ENCRYPTED_DATA_INVALID", 0 },
		{ CKR_ENCRYPTED_DATA_LEN_RANGE          , "CKR_ENCRYPTED_DATA_LEN_RANGE", 0 },
		{ CKR_FUNCTION_CANCELED                 , "CKR_FUNCTION_CANCELED", 0 },
		{ CKR_FUNCTION_NOT_PARALLEL             , "CKR_FUNCTION_NOT_PARALLEL", 0 },
		{ CKR_FUNCTION_NOT_SUPPORTED            , "CKR_FUNCTION_NOT_SUPPORTED", 0 },
		{ CKR_KEY_HANDLE_INVALID                , "CKR_KEY_HANDLE_INVALID", 0 },
		{ CKR_KEY_SIZE_RANGE                    , "CKR_KEY_SIZE_RANGE", 0 },
		{ CKR_KEY_TYPE_INCONSISTENT             , "CKR_KEY_TYPE_INCONSISTENT", 0 },
		{ CKR_KEY_NOT_NEEDED                    , "CKR_KEY_NOT_NEEDED", 0 },
		{ CKR_KEY_CHANGED                       , "CKR_KEY_CHANGED", 0 },
		{ CKR_KEY_NEEDED                        , "CKR_KEY_NEEDED", 0 },
		{ CKR_KEY_INDIGESTIBLE                  , "CKR_KEY_INDIGESTIBLE", 0 },
		{ CKR_KEY_FUNCTION_NOT_PERMITTED        , "CKR_KEY_FUNCTION_NOT_PERMITTED", 0 },
		{ CKR_KEY_NOT_WRAPPABLE                 , "CKR_KEY_NOT_WRAPPABLE", 0 },
		{ CKR_KEY_UNEXTRACTABLE                 , "CKR_KEY_UNEXTRACTABLE", 0 },
		{ CKR_MECHANISM_INVALID                 , "CKR_MECHANISM_INVALID", 0 },
		{ CKR_MECHANISM_PARAM_INVALID           , "CKR_MECHANISM_PARAM_INVALID", 0 },
		{ CKR_OBJECT_HANDLE_INVALID             , "CKR_OBJECT_HANDLE_INVALID", 0 },
		{ CKR_OPERATION_ACTIVE                  , "CKR_OPERATION_ACTIVE", 0 },
		{ CKR_OPERATION_NOT_INITIALIZED         , "CKR_OPERATION_NOT_INITIALIZED", 0 },
		{ CKR_PIN_INCORRECT                     , "CKR_PIN_INCORRECT", 0 },
		{ CKR_PIN_INVALID                       , "CKR_PIN_INVALID", 0 },
		{ CKR_PIN_LEN_RANGE                     , "CKR_PIN_LEN_RANGE", 0 },
		{ CKR_PIN_EXPIRED                       , "CKR_PIN_EXPIRED", 0 },
		{ CKR_PIN_LOCKED                        , "CKR_PIN_LOCKED", 0 },
		{ CKR_SESSION_CLOSED                    , "CKR_SESSION_CLOSED", 0 },
		{ CKR_SESSION_COUNT                     , "CKR_SESSION_COUNT", 0 },
		{ CKR_SESSION_HANDLE_INVALID            , "CKR_SESSION_HANDLE_INVALID", 0 },
		{ CKR_SESSION_PARALLEL_NOT_SUPPORTED    , "CKR_SESSION_PARALLEL_NOT_SUPPORTED", 0 },
		{ CKR_SESSION_READ_ONLY                 , "CKR_SESSION_READ_ONLY", 0 },
		{ CKR_SESSION_EXISTS                    , "CKR_SESSION_EXISTS", 0 },
		{ CKR_SESSION_READ_ONLY_EXISTS          , "CKR_SESSION_READ_ONLY_EXISTS", 0 },
		{ CKR_SESSION_READ_WRITE_SO_EXISTS      , "CKR_SESSION_READ_WRITE_SO_EXISTS", 0 },
		{ CKR_SIGNATURE_INVALID                 , "CKR_SIGNATURE_INVALID", 0 },
		{ CKR_SIGNATURE_LEN_RANGE               , "CKR_SIGNATURE_LEN_RANGE", 0 },
		{ CKR_TEMPLATE_INCOMPLETE               , "CKR_TEMPLATE_INCOMPLETE", 0 },
		{ CKR_TEMPLATE_INCONSISTENT             , "CKR_TEMPLATE_INCONSISTENT", 0 },
		{ CKR_TOKEN_NOT_PRESENT                 , "CKR_TOKEN_NOT_PRESENT", 0 },
		{ CKR_TOKEN_NOT_RECOGNIZED              , "CKR_TOKEN_NOT_RECOGNIZED", 0 },
		{ CKR_TOKEN_WRITE_PROTECTED             , "CKR_TOKEN_WRITE_PROTECTED", 0 },
		{ CKR_UNWRAPPING_KEY_HANDLE_INVALID     , "CKR_UNWRAPPING_KEY_HANDLE_INVALID", 0 },
		{ CKR_UNWRAPPING_KEY_SIZE_RANGE         , "CKR_UNWRAPPING_KEY_SIZE_RANGE", 0 },
		{ CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT  , "CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT", 0 },
		{ CKR_USER_ALREADY_LOGGED_IN            , "CKR_USER_ALREADY_LOGGED_IN", 0 },
		{ CKR_USER_NOT_LOGGED_IN                , "CKR_USER_NOT_LOGGED_IN", 0 },
		{ CKR_USER_PIN_NOT_INITIALIZED          , "CKR_USER_PIN_NOT_INITIALIZED", 0 },
		{ CKR_USER_TYPE_INVALID                 , "CKR_USER_TYPE_INVALID", 0 },
		{ CKR_USER_ANOTHER_ALREADY_LOGGED_IN    , "CKR_USER_ANOTHER_ALREADY_LOGGED_IN", 0 },
		{ CKR_USER_TOO_MANY_TYPES               , "CKR_USER_TOO_MANY_TYPES", 0 },
		{ CKR_WRAPPED_KEY_INVALID               , "CKR_WRAPPED_KEY_INVALID", 0 },
		{ CKR_WRAPPED_KEY_LEN_RANGE             , "CKR_WRAPPED_KEY_LEN_RANGE", 0 },
		{ CKR_WRAPPING_KEY_HANDLE_INVALID       , "CKR_WRAPPING_KEY_HANDLE_INVALID", 0 },
		{ CKR_WRAPPING_KEY_SIZE_RANGE           , "CKR_WRAPPING_KEY_SIZE_RANGE", 0 },
		{ CKR_WRAPPING_KEY_TYPE_INCONSISTENT    , "CKR_WRAPPING_KEY_TYPE_INCONSISTENT", 0 },
		{ CKR_RANDOM_SEED_NOT_SUPPORTED         , "CKR_RANDOM_SEED_NOT_SUPPORTED", 0 },
		{ CKR_RANDOM_NO_RNG                     , "CKR_RANDOM_NO_RNG", 0 },
		{ CKR_DOMAIN_PARAMS_INVALID             , "CKR_DOMAIN_PARAMS_INVALID", 0 },
		{ CKR_BUFFER_TOO_SMALL                  , "CKR_BUFFER_TOO_SMALL", 0 },
		{ CKR_SAVED_STATE_INVALID               , "CKR_SAVED_STATE_INVALID", 0 },
		{ CKR_INFORMATION_SENSITIVE             , "CKR_INFORMATION_SENSITIVE", 0 },
		{ CKR_STATE_UNSAVEABLE                  , "CKR_STATE_UNSAVEABLE", 0 },
		{ CKR_CRYPTOKI_NOT_INITIALIZED          , "CKR_CRYPTOKI_NOT_INITIALIZED", 0 },
		{ CKR_CRYPTOKI_ALREADY_INITIALIZED      , "CKR_CRYPTOKI_ALREADY_INITIALIZED", 0 },
		{ CKR_MUTEX_BAD                         , "CKR_MUTEX_BAD", 0 },
		{ CKR_MUTEX_NOT_LOCKED                  , "CKR_MUTEX_NOT_LOCKED", 0 },
		{ CKR_OK			                    , "CKR_OK", 0 },
		{ 0, NULL }
};


#define CKT_BBOOL       1
#define CKT_BIN         2
#define CKT_DATE        3
#define CKT_LONG        4
#define CKT_ULONG       5

#define P11CKA			58

struct id2name_t p11CKAName[P11CKA + 1] = {
		{ CKA_CLASS                              , "CKA_CLASS", CKT_LONG },
		{ CKA_TOKEN                              , "CKA_TOKEN", CKT_BBOOL },
		{ CKA_PRIVATE                            , "CKA_PRIVATE", CKT_BBOOL },
		{ CKA_LABEL                              , "CKA_LABEL", 0 },
		{ CKA_APPLICATION                        , "CKA_APPLICATION", 0 },
		{ CKA_VALUE                              , "CKA_VALUE", CKT_BIN },
		{ CKA_OBJECT_ID                          , "CKA_OBJECT_ID", 0 },
		{ CKA_CERTIFICATE_TYPE                   , "CKA_CERTIFICATE_TYPE", CKT_ULONG },
		{ CKA_ISSUER                             , "CKA_ISSUER", 0 },
		{ CKA_SERIAL_NUMBER                      , "CKA_SERIAL_NUMBER", 0 },
		{ CKA_AC_ISSUER                          , "CKA_AC_ISSUER", 0 },
		{ CKA_OWNER                              , "CKA_OWNER", 0 },
		{ CKA_ATTR_TYPES                         , "CKA_ATTR_TYPES", 0 },
		{ CKA_TRUSTED                            , "CKA_TRUSTED", 0 },
		{ CKA_KEY_TYPE                           , "CKA_KEY_TYPE", 0 },
		{ CKA_SUBJECT                            , "CKA_SUBJECT", 0 },
		{ CKA_ID                                 , "CKA_ID", CKT_BIN },
		{ CKA_SENSITIVE                          , "CKA_SENSITIVE", CKT_BBOOL },
		{ CKA_ENCRYPT                            , "CKA_ENCRYPT", CKT_BBOOL },
		{ CKA_DECRYPT                            , "CKA_DECRYPT", CKT_BBOOL },
		{ CKA_WRAP                               , "CKA_WRAP", CKT_BBOOL },
		{ CKA_UNWRAP                             , "CKA_UNWRAP", CKT_BBOOL },
		{ CKA_SIGN                               , "CKA_SIGN", CKT_BBOOL },
		{ CKA_SIGN_RECOVER                       , "CKA_SIGN_RECOVER", CKT_BBOOL },
		{ CKA_VERIFY                             , "CKA_VERIFY", CKT_BBOOL },
		{ CKA_VERIFY_RECOVER                     , "CKA_VERIFY_RECOVER", 0 },
		{ CKA_DERIVE                             , "CKA_DERIVE", CKT_BBOOL },
		{ CKA_START_DATE                         , "CKA_START_DATE", CKT_DATE },
		{ CKA_END_DATE                           , "CKA_END_DATE", CKT_DATE },
		{ CKA_MODULUS                            , "CKA_MODULUS", 0 },
		{ CKA_MODULUS_BITS                       , "CKA_MODULUS_BITS", 0 },
		{ CKA_PUBLIC_EXPONENT                    , "CKA_PUBLIC_EXPONENT", 0 },
		{ CKA_PRIVATE_EXPONENT                   , "CKA_PRIVATE_EXPONENT", 0 },
		{ CKA_PRIME_1                            , "CKA_PRIME_1", 0 },
		{ CKA_PRIME_2                            , "CKA_PRIME_2", 0 },
		{ CKA_EXPONENT_1                         , "CKA_EXPONENT_1", 0 },
		{ CKA_EXPONENT_2                         , "CKA_EXPONENT_2", 0 },
		{ CKA_COEFFICIENT                        , "CKA_COEFFICIENT", 0 },
		{ CKA_PRIME                              , "CKA_PRIME", 0 },
		{ CKA_SUBPRIME                           , "CKA_SUBPRIME", 0 },
		{ CKA_BASE                               , "CKA_BASE", 0 },
		{ CKA_PRIME_BITS                         , "CKA_PRIME_BITS", 0 },
		{ CKA_SUBPRIME_BITS                      , "CKA_SUBPRIME_BITS", 0 },
		{ CKA_VALUE_BITS                         , "CKA_VALUE_BITS", 0 },
		{ CKA_VALUE_LEN                          , "CKA_VALUE_LEN", CKT_LONG },
		{ CKA_EXTRACTABLE                        , "CKA_EXTRACTABLE", CKT_BBOOL },
		{ CKA_LOCAL                              , "CKA_LOCAL", CKT_BBOOL },
		{ CKA_NEVER_EXTRACTABLE                  , "CKA_NEVER_EXTRACTABLE", CKT_BBOOL },
		{ CKA_ALWAYS_SENSITIVE                   , "CKA_ALWAYS_SENSITIVE", CKT_BBOOL },
		{ CKA_KEY_GEN_MECHANISM                  , "CKA_KEY_GEN_MECHANISM", CKT_LONG },
		{ CKA_MODIFIABLE                         , "CKA_MODIFIABLE", CKT_BBOOL },
		{ CKA_EC_PARAMS                          , "CKA_EC_PARAMS", 0 },
		{ CKA_EC_POINT                           , "CKA_EC_POINT", 0 },
		{ CKA_SECONDARY_AUTH                     , "CKA_SECONDARY_AUTH", 0 },
		{ CKA_AUTH_PIN_FLAGS                     , "CKA_AUTH_PIN_FLAGS", 0 },
		{ CKA_HW_FEATURE_TYPE                    , "CKA_HW_FEATURE_TYPE", 0 },
		{ CKA_RESET_ON_INIT                      , "CKA_RESET_ON_INIT", 0 },
		{ CKA_HAS_RESET                          , "CKA_HAS_RESET", 0 },
		{ 0, NULL }
};

struct id2name_t p11CKKName[] = {
		{ CKK_RSA                                , "CKK_RSA", 0 },
		{ CKK_DSA                                , "CKK_DSA", 0 },
		{ CKK_DH                                 , "CKK_DH", 0 },
		{ CKK_EC                                 , "CKK_EC", 0 },
		{ CKK_X9_42_DH                           , "CKK_X9_42_DH", 0 },
		{ CKK_KEA                                , "CKK_KEA", 0 },
		{ CKK_GENERIC_SECRET                     , "CKK_GENERIC_SECRET", 0 },
		{ CKK_RC2                                , "CKK_RC2", 0 },
		{ CKK_RC4                                , "CKK_RC4", 0 },
		{ CKK_DES                                , "CKK_DES", 0 },
		{ CKK_DES2                               , "CKK_DES2", 0 },
		{ CKK_DES3                               , "CKK_DES3", 0 },
		{ CKK_CAST                               , "CKK_CAST", 0 },
		{ CKK_CAST3                              , "CKK_CAST3", 0 },
		{ CKK_CAST128                            , "CKK_CAST128", 0 },
		{ CKK_RC5                                , "CKK_RC5", 0 },
		{ CKK_IDEA                               , "CKK_IDEA", 0 },
		{ CKK_SKIPJACK                           , "CKK_SKIPJACK", 0 },
		{ CKK_BATON                              , "CKK_BATON", 0 },
		{ CKK_JUNIPER                            , "CKK_JUNIPER", 0 },
		{ CKK_CDMF                               , "CKK_CDMF", 0 },
		{ CKK_AES                                , "CKK_AES", 0 },
		{ 0, NULL }
};

/* Data structure for parameters passed to thread */
struct thread_data {
	CK_FUNCTION_LIST_PTR p11;
	CK_KEY_TYPE keytype;
	CK_SLOT_ID slotid;
	int  threadno;
	SEMAPHORE *psem;
};


char *p11libname = P11LIBNAME;

CK_UTF8CHAR *pin = (CK_UTF8CHAR *)PIN;
CK_UTF8CHAR wrongpin[] = "111111";
CK_ULONG pinlen = 6;

CK_UTF8CHAR sopin[] = "3537363231383830";
CK_ULONG sopinlen = 16;

static unsigned testsCompleted = 0;
static unsigned testsFailed = 0;
static unsigned tooManyKeysReported = 0;
static unsigned signCount;

static int optTestInsertRemove = 0;
static int optTestRSADecryption = 0;
static int optTestPINBlock = 0;
static int optTestMultiOnly = 0;
static int optOneThreadPerToken = 0;
static int optMaxParallelThreads = 10;
static int optThreadTestTime = 10000; /* 10 seconds */

static int requestClose;

static char namebuf[40]; /* used by main thread */

static char *verdict(int condition)
{
	char* result;
	if (requestClose) {
		return "Aborting";
	}
	InterlockedIncrement(&testsCompleted);
	if (!condition) {
		InterlockedIncrement(&testsFailed);
		return "Failed";
	}
	return "Passed";
}


static char *id2name(struct id2name_t *p, unsigned long id, unsigned long *pAttr, char scr[40]) {

	if (pAttr)
		*pAttr = 0;

	if (id & 0x80000000) {
		sprintf(scr, "Vendor defined 0x%lx", id);
	} else {
		while (p->name && (p->id != id)) {
			p++;
		}

		if (p->name) {
			strcpy(scr, p->name);
			if (pAttr)
				*pAttr = p->attr;
		} else {
			sprintf(scr, "*** Undefined 0x%lx ***", id);
		}
	}
	return scr;
}

#define CKR_Name(i)    id2name(p11CKRName, i, 0, namebuf)
#define CKA_Name(i, j) id2name(p11CKAName, i, j, namebuf)
#define CKK_Name(i)    id2name(p11CKKName, i, 0, namebuf)


static char *p11string(CK_UTF8CHAR *str, size_t len)
{
	static char buffer[81]; /* not thread safe */
	int i;

	if (len + 1 > sizeof(buffer))
		return "**Input too long***";

	memcpy(buffer, str, len);
	buffer[len] = 0;

	/* right trim buffer */
	for (i = len; --i >= 0 && buffer[i] == ' '; ) { /* canonical reverse array iteration */
		buffer[i] = 0;
	}
	return buffer;
}



static void bin2str(char *st, int stlen, unsigned char *data, int datalen)
{
	int ascii, i;
	unsigned char *d;

	ascii = 1;
	d = data;
	i = datalen;

	while (i && (stlen > 2)) {
		sprintf(st, "%02X", *d);

		if (ascii && !isprint(*d) && *d)
			ascii = 0;

		st += 2;
		stlen -= 2;
		i--;
		d++;
	}

	if (ascii && (stlen > datalen + 3)) {
		*st++ = ' ';
		*st++ = '"';
		memcpy(st, data, datalen);
		st += datalen;
		*st++ = '"';
	}

	*st = '\0';
}



void dumpAttribute(CK_ATTRIBUTE_PTR attr)
{
	char attribute[30], scr[4096];
	unsigned long atype;

	strcpy(attribute, CKA_Name(attr->type, &atype));

	switch(attr->type) {

	case CKA_KEY_TYPE:
		printf("  %s = %s\n", attribute, CKK_Name(*(CK_KEY_TYPE *)attr->pValue));
		break;

	default:
		switch(atype) {
		case CKT_BBOOL:
			if (attr->pValue) {
				printf("  %s = %s [%2d]\n", attribute, *(CK_BBOOL *)attr->pValue ? "TRUE" : "FALSE", *(CK_BBOOL *)attr->pValue);
			} else {
				printf("  %s\n", attribute);
			}
			break;
		case CKT_DATE:
			// pdate = (CK_DATE *)attr->pValue;
			// if (pdate != NULL) {
			//     sprintf(res, "  %s = %4s-%2s-%2s", attribute, pdate->year, pdate->month, pdate->day);
			// }
			printf("  %s\n", attribute);
			break;
		case CKT_LONG:
			printf("  %s = %d [0x%X]\n", attribute, (int)*(CK_LONG *)attr->pValue, (int)*(CK_LONG *)attr->pValue);
			break;
		case CKT_ULONG:
			printf("  %s = %u [0x%X]\n", attribute, (unsigned)*(CK_ULONG *)attr->pValue, (unsigned)*(CK_ULONG *)attr->pValue);
			break;
		case CKT_BIN:
		default:
			bin2str(scr, sizeof(scr), (unsigned char*)attr->pValue, attr->ulValueLen);
			printf("  %s = %s\n", attribute, scr);
			break;
		}
	}
}



void dumpObject(CK_FUNCTION_LIST_PTR p11, CK_SESSION_HANDLE session, CK_OBJECT_HANDLE hnd)
{
	/* windows intellisence does not like reserved words */
	CK_ATTRIBUTE _template[P11CKA];
	int rc, i;

	memset(_template, 0, sizeof(_template));
	for (i = 0; i < P11CKA; i++) {
		_template[i].type = p11CKAName[i].id;
	}
	printf("Calling C_GetAttributeValue ");
	rc = p11->C_GetAttributeValue(session, hnd, (CK_ATTRIBUTE_PTR)&_template, P11CKA);
	printf("- %s : %s\n", CKR_Name(rc), (rc == CKR_OK) || (rc == CKR_ATTRIBUTE_TYPE_INVALID) ? "Passed" : "Failed");

	for (i = 0; i < P11CKA; i++) {
		if ((CK_LONG)_template[i].ulValueLen > 0) {
			_template[i].pValue = alloca(_template[i].ulValueLen);
		}
	}

	printf("Calling C_GetAttributeValue ");
	rc = p11->C_GetAttributeValue(session, hnd, (CK_ATTRIBUTE_PTR)&_template, P11CKA);
	printf("- %s : %s\n", CKR_Name(rc), (rc == CKR_OK) || (rc == CKR_ATTRIBUTE_TYPE_INVALID) ? "Passed" : "Failed");

	for (i = 0; i < P11CKA; i++) {
		if ((CK_LONG)_template[i].ulValueLen > 0) {
			dumpAttribute(&_template[i]);
		}
	}
}



void listObjects(CK_FUNCTION_LIST_PTR p11, CK_SESSION_HANDLE session, CK_ATTRIBUTE_PTR attr, int len)
{
	CK_OBJECT_HANDLE hnd;
	CK_ULONG cnt;
	int rc;

	printf("Calling C_FindObjectsInit ");
	rc = p11->C_FindObjectsInit(session, attr, len);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		return;
	}

	for (;;) {
		printf("Calling C_FindObjects ");
		rc = p11->C_FindObjects(session, &hnd, 1, &cnt);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		if (!(rc == CKR_OK && cnt > 0)) {
			break;
		}
		dumpObject(p11, session, hnd);
	}

	printf("Calling C_FindObjectsFinal ");
	p11->C_FindObjectsFinal(session);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
}


/* suppress output for all but the main thread */
int findObjectAtOffset(CK_FUNCTION_LIST_PTR p11, CK_SESSION_HANDLE session,
	CK_ATTRIBUTE_PTR attr, int attrlen,
	int off, CK_OBJECT_HANDLE_PTR phnd, int threadno)
{
	CK_ULONG cnt;
	CK_OBJECT_HANDLE hnd;
	int rc, off0 = off;

#define printf0 if (threadno == 0) printf

	printf0("[%2d] Calling C_FindObjectsInit\n", threadno);
	rc = p11->C_FindObjectsInit(session, attr, attrlen);
	printf0("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		return rc;
	}

	do {
		printf0("[%2d] Calling C_FindObjects\n", threadno);
		cnt = 1;
		rc = p11->C_FindObjects(session, &hnd, 1, &cnt);
		printf0("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
	} while (rc == CKR_OK && cnt == 1 && off-- > 0); 

	printf0("[%2d] Calling C_FindObjectsFinal\n", threadno);
	rc = p11->C_FindObjectsFinal(session);
	printf0("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK || cnt == 0) {
		*phnd = CK_INVALID_HANDLE;
		return CKR_FUNCTION_FAILED;
	}

	*phnd = hnd;
	return CKR_OK;

#undef printf0

}



int testSigning(CK_FUNCTION_LIST_PTR p11, CK_SLOT_ID slotid, CK_KEY_TYPE keytype, int threadno)
{
	CK_SESSION_HANDLE session;
	/* windows intellisence does not like reserved words */
	CK_OBJECT_CLASS _class = CKO_PRIVATE_KEY;
	CK_ATTRIBUTE _template[] = {
			{ CKA_CLASS, &_class, sizeof(_class) },
			{ CKA_KEY_TYPE, &keytype, sizeof(keytype) }
	};
	CK_OBJECT_HANDLE hnd;
	CK_MECHANISM mech = { keytype == CKK_RSA ? CKM_SHA1_RSA_PKCS : CKM_ECDSA_SHA1, 0, 0 };
	CK_BYTE_PTR text = (CK_BYTE_PTR)"Hello World";
	int textlen = strlen((char*)text);
	CK_BYTE signature[256]; /* space for RSA 2K */
	CK_ULONG len;
	char scr[1024];
	int rc, keyno, i;
	char namebuf[40]; /* each thread need its own buffer */

	printf("[%2d] Calling C_OpenSession(Slot=%ld)\n", threadno, slotid);
	rc = p11->C_OpenSession(slotid, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session);
	printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
	if (rc != CKR_OK || requestClose) {
		printf("[%2d] Thread aborting (cannot open session)\n", threadno);
		return rc;
	}

	printf("[%2d] Calling C_Login User\n", threadno);
	rc = p11->C_Login(session, CKU_USER, pin, pinlen);
	/* another thread could have logged in on this slot => CKR_USER_ALREADY_LOGGED_IN is ok here */
	printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK || rc == CKR_USER_ALREADY_LOGGED_IN));
	if (rc == CKR_USER_ALREADY_LOGGED_IN)
		rc = CKR_OK;

	for (keyno = 0; rc == CKR_OK && !requestClose; keyno++) {

		printf("[%2d] Calling findObjectAtOffset(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		rc = findObjectAtOffset(p11, session, (CK_ATTRIBUTE_PTR)&_template, sizeof(_template) / sizeof(_template[0]), keyno, &hnd, threadno);

		if (rc == CKR_OK) {
			printf("[%2d] Key %i found(Session %ld, Slot=%ld)\n", threadno, keyno, session, slotid);
			if (keyno > 0) {
				InterlockedIncrement(&tooManyKeysReported);
				printf("[%2d] ************ Corruption detected ************\n", threadno);
			}
		} else {
			printf("[%2d] Key %i not found(Session %ld, Slot=%ld)\n", threadno, keyno, session, slotid);
			break;
		}

		printf("[%2d] Calling C_SignInit(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		rc = p11->C_SignInit(session, &mech, hnd);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (rc || requestClose)
			break;
#if 0
		printf("[%2d] Calling C_Sign(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		len = 0;
		rc = p11->C_Sign(session, text, textlen, NULL, &len);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (rc || requestClose)
			break;
		printf("[%2d] Signature size = %lu\n", threadno, len);
#endif
		printf("[%2d] Calling C_Sign(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		len = sizeof(signature);
		rc = p11->C_Sign(session, text, textlen, signature, &len);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (requestClose)
			break;
		if (rc == CKR_OK)
			InterlockedIncrement(&signCount);
#if 0
		bin2str(scr, sizeof(scr), signature, len);
		printf("[%2d] Signature:\n%s\n", threadno, scr);
#endif
		printf("[%2d] Calling C_SignInit(Session %ld, Slot=%ld - Multipart)\n", threadno, session, slotid);
		rc = p11->C_SignInit(session, &mech, hnd);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (rc || requestClose)
			break;
	
		printf("[%2d] Calling C_SignUpdate(Session %ld, Slot=%ld - Part #1)\n", threadno, session, slotid);
		rc = p11->C_SignUpdate(session, text, 6);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (rc || requestClose)
			break;

		printf("[%2d] Calling C_SignUpdate(Session %ld, Slot=%ld - Part #2)\n", threadno, session, slotid);
		rc = p11->C_SignUpdate(session, text + 6, textlen - 6);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (rc || requestClose)
			break;
#if 0
		printf("[%2d] Calling C_SignFinal(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		len = 0;
		rc = p11->C_SignFinal(session, NULL, &len);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		printf("[%2d] Signature size = %lu\n", threadno, len);
		if (rc || requestClose)
			break;
#endif
		printf("[%2d] Calling C_SignFinal(Session %ld, Slot=%ld)\n", threadno, session, slotid);
		len = sizeof(signature);
		rc = p11->C_SignFinal(session, signature, &len);
		printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));
		if (requestClose)
			break;
		if (rc == CKR_OK)
			InterlockedIncrement(&signCount);
#if 0
		bin2str(scr, sizeof(scr), signature, len);
		printf("[%2d] Signature:\n%s\n", threadno, scr);
#endif
	}

	printf("[%2d] Calling C_CloseSession(Session %ld, Slot=%ld)\n", threadno, session, slotid);
	rc = p11->C_CloseSession(session);
	printf("[%2d] - %s : %s\n", threadno, CKR_Name(rc), verdict(rc == CKR_OK));

	return 0;
}


#ifndef _WIN32
void*
#else
DWORD WINAPI
#endif
SignThreadFunc(void *arg)
{
	struct thread_data *ptd = (struct thread_data *)arg;
	CK_FUNCTION_LIST_PTR p11 = ptd->p11;
	CK_KEY_TYPE keytype      = ptd->keytype;
	int threadno             = ptd->threadno;
	SEMAPHORE *psem          = ptd->psem;
	CK_SLOT_ID slotid        = ptd->slotid;
	free(arg);

	testSigning(p11, slotid, keytype, threadno);
	printf("[%2d] Thread completed\n", threadno);

	sem_post(psem);
	return 0;
}



void testSigningMultiThreaded(CK_FUNCTION_LIST_PTR p11, CK_KEY_TYPE keytype,
	unsigned *pSignCount, unsigned *pTokenCount, long *pElapsed)
{
	CK_ULONG slotcount;
	CK_SLOT_ID slotids[256];
	SEMAPHORE sem;
	long start, stop;
	void *status;
	int rc, threadno, parallelThreads;
	int i, slotix;

	printf("[ 0] Calling C_GetSlotList ");
	slotcount = sizeof(slotids) / sizeof(slotids[0]);
	rc = p11->C_GetSlotList(TRUE, slotids, &slotcount);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	if (rc != CKR_OK || slotcount <= 0)
		return;
	*pTokenCount = slotcount;

#if 1
	/* open a session on each slot (just to keep the session count >1 on each slot to prevent implicit log out) */
	for (i = 0; i < slotcount; i++) {
		CK_SESSION_HANDLE session;
		printf("[ 0] Calling C_OpenSession(Slot=%ld)\n", slotids[i]);
		rc = p11->C_OpenSession(slotids[i], CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session);
		printf("[ 0] - %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	}
#endif

	parallelThreads = optOneThreadPerToken ? slotcount : optMaxParallelThreads;
	assert(!sem_init(&sem, 0, parallelThreads));
	/* reset global counter */
	signCount = 0;
	start = GetTickCount();
	/* run for optThreadTestTime ms */
	threadno = 1; /* 0 is main thread */
	slotix = 0;
	while (!requestClose && GetTickCount() - start < optThreadTestTime) {
		CK_SLOT_ID slotid;
		int tokenfound = FALSE;
		printf("[ 0] Calling C_GetSlotList\n");
		slotcount = sizeof(slotids) / sizeof(slotids[0]);
		rc = p11->C_GetSlotList(FALSE, slotids, &slotcount);
		printf("[ 0] - %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		for (i = 0; i < slotcount; i++) {
			CK_SLOT_INFO slotinfo;
			if (GetTickCount() - start >= optThreadTestTime) {
				tokenfound = TRUE;
				break;
			}
			if (slotix >= slotcount) {
				slotix = 0;
			}
			slotid = slotids[slotix++];
			printf("[ 0] Calling C_GetSlotInfo for slot %lu\n", slotid);
			rc = p11->C_GetSlotInfo(slotid, &slotinfo);
			printf("[ 0] - %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
			if (rc == 0 && (slotinfo.flags & CKF_TOKEN_PRESENT)) {
				pthread_t thread;
				struct thread_data* ptd = (struct thread_data *)malloc(sizeof(*ptd));
				assert(ptd);
				tokenfound = TRUE;
				ptd->p11 = p11;
				ptd->threadno = threadno++;
				ptd->keytype = keytype;
				ptd->slotid = slotid;
				ptd->psem = &sem;
				sem_wait(&sem);
				rc = pthread_create(&thread, 0, SignThreadFunc, ptd);
				if (rc) {
					free(ptd);
					sem_post(&sem);
					printf("ERROR: create thread returns %d\n", rc);
				} else {
					assert(!pthread_detach(thread));
				}
			}
		}
		if (!tokenfound) {
			usleep(1000000);
			continue;
		}
	}

	requestClose++;
	stop = GetTickCount();
	*pSignCount = signCount;
	*pElapsed = stop - start;

	for (i = 0; i < slotcount; i++) {
		printf("[ 0] Calling C_CloseAllSessions\n");
		rc = p11->C_CloseAllSessions(slotids[i]);
		printf("[ 0] - %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	}

	/* wait for termination of all threads */
	for (i = 0; i < parallelThreads; i++) {
		sem_wait(&sem);
	}

	/* release sem before destroy */
	for (i = 0; i < parallelThreads; i++) {
		sem_post(&sem);
	}
	requestClose--;

	assert(!sem_destroy(&sem));
}



void testRSADecryption(CK_FUNCTION_LIST_PTR p11, CK_SESSION_HANDLE session)
{
	CK_OBJECT_CLASS privKey = CKO_PRIVATE_KEY;
	CK_KEY_TYPE keytype = CKK_RSA;
	CK_CHAR label[] = "Joe Doe (RSA2048)";
	CK_ATTRIBUTE _template[] = {
			{ CKA_CLASS, &privKey, sizeof(privKey) },
			{ CKA_KEY_TYPE, &keytype, sizeof(keytype) },
			{ CKA_LABEL, &label, strlen((char *)label) }
	};
	CK_OBJECT_HANDLE hnd;
	CK_MECHANISM mech_raw = { CKM_RSA_X_509, 0, 0 };
	CK_MECHANISM mech_p15 = { CKM_RSA_PKCS, 0, 0 };
	// Place valid cryptograms from use case tests here
	char *raw_cryptogram = "\xCD\x6A\x28\xD1\x4A\x4A\x07\xED\x33\x24\x61\xFC\xF7\x3A\x51\x1B\x4F\x15\xF7\xC6\x95\xFC\xB4\xBE\x00\xE4\xA1\x17\x95\x98\x2F\xB5\x7A\x26\xB7\xDA\xF9\x31\x9F\xA9\xB0\xBE\xF9\xCB\x94\xFF\x88\xF1\x4D\x35\x57\xF8\x56\x51\xAF\xD9\x00\xB0\x3C\xE3\x82\x8E\xF1\xC9\xED\x68\x95\xAF\xDE\xF1\x6D\x7C\x67\x39\x3C\x68\xD9\x02\xFD\x39\x24\x15\xA3\x66\x03\xB9\x9E\x96\xAC\x28\x50\x02\xC9\x0E\x87\x92\xDC\x3B\x9E\x35\x6E\x06\x79\xB7\xBC\x9F\x68\x5A\xAA\xC0\x08\x0F\xB4\x92\xC7\xC1\xE6\xCE\x17\xBC\xB8\x16\xF5\xBD\x41\x7E\x10\xC6\x51\xC5\xA2\x12\x89\xE5\x8A\x7F\x98\xCA\x6A\x44\x5D\x9E\x5B\x9C\xA3\xB6\x64\x52\xD0\xF1\xA1\x9D\xC3\x81\x89\xB5\x6E\xB6\xB8\x0C\x4B\xB1\x31\xD1\x37\x68\x2F\xB4\x0F\x7F\x03\x2F\x8A\x65\x7F\x98\xDF\x05\x15\x78\xC5\x14\x00\xB9\xF2\x82\x3A\xDA\x62\x85\xAF\xAB\x7C\x5B\x7E\x2F\x7C\xE4\xCA\xB0\xE5\xD7\x3A\x6D\x68\x5C\x48\x16\x4B\x36\x2E\xD9\xF3\xC7\x88\x11\x0B\x6B\xBB\x50\x39\x3D\x6C\x20\x24\x5E\x1C\x83\x80\x13\x3E\x59\x62\xEF\x94\x1D\xC9\x9D\x40\x18\x14\x51\x1E\x80\x07\x30\x74\x4A\xD9\x16\xFA\xFF\x60\x4B\x5C\xE4";
	char *p15_cryptogram = "\xAA\x80\xBF\x66\x99\x0A\x6E\xF3\x83\xA2\x7B\x2F\x89\x56\x0F\x7D\xC7\xFD\x44\x36\x86\x56\xC5\xC6\xA3\x3E\x89\xFC\x37\x87\x8A\xB0\xD5\xEB\x46\x20\x1D\xE4\xB7\xA7\xDE\xAC\x1E\x70\xBD\x66\x97\x91\xA3\xAC\xFA\x70\x80\x27\x8E\x7E\x8C\x06\x23\xA1\xB6\x83\x1A\x04\x96\xE7\x87\x1C\x61\xEC\xE0\x1A\x7D\xA9\x85\x85\x75\xBB\xDA\x77\x07\x65\x2A\x7A\x27\xCC\x14\xE4\x34\xBC\x70\xDF\x46\x67\xA0\x5B\x62\x2C\xF7\x2D\xFD\xF7\xA7\xFF\x89\x16\xC0\xE3\x2B\xEF\xDB\x1E\x11\x2A\xAE\x81\xDE\xDA\x96\xE4\xD3\xE4\x31\xE8\x31\xE9\xFD\xCD\x48\x0B\x9D\x95\xC0\x45\x14\x38\x03\x41\x00\xB0\xF9\xF0\x5A\x22\xBF\x2D\x81\xB4\x20\x7E\x05\x68\x90\x2D\x67\x9E\xEA\xC1\xFC\x7C\x92\x99\xD1\xDE\xE7\xEA\xE3\x0A\x14\x52\x19\xD0\x7C\xDE\x8C\x37\xBC\xA6\x52\xAB\x3D\x7A\xAE\x60\x11\xC7\x41\xAB\x53\x48\x08\xBA\xC6\x80\xC3\x72\xB7\x13\x15\xD7\x7E\x40\x8C\x0E\x29\x33\xB4\x11\xBB\x1B\x96\x7B\x2A\x52\x98\x24\xEE\xC0\x51\xD7\x55\x25\x59\x55\xD8\xB3\xAB\x06\x26\x28\x7F\x0F\xB2\x44\xF3\xBA\xEE\xA7\xA2\xDB\xAA\xD2\xE7\xB7\x79\x51\xB2\xFB\x1B\x7F\x1D\xE4\xA7\x08\x7D\xAF";
	CK_BYTE plain[256];
	CK_ULONG len;
	char scr[1024];
	int rc;

	rc = findObjectAtOffset(p11, session, (CK_ATTRIBUTE_PTR)&_template, sizeof(_template) / sizeof(CK_ATTRIBUTE), 0, &hnd, 0);

	if (rc != CKR_OK) {
		printf("Key %s not found\n", label);
		return;
	}

	printf("Calling C_DecryptInit() ");
	rc = p11->C_DecryptInit(session, &mech_raw, hnd);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_Decrypt() ");

	rc = p11->C_Decrypt(session, (CK_BYTE_PTR)raw_cryptogram, 256, NULL, &len);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Plain size = %lu\n", len);

	len = sizeof(plain);
	len = 0;
	rc = p11->C_Decrypt(session, (CK_BYTE_PTR)raw_cryptogram, 256, plain, &len);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	bin2str(scr, sizeof(scr), plain, len);
	printf("Plain:\n%s\n", scr);


	printf("Calling C_DecryptInit() ");
	rc = p11->C_DecryptInit(session, &mech_p15, hnd);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_Decrypt() ");
	len = 0;
	rc = p11->C_Decrypt(session, (CK_BYTE_PTR)p15_cryptogram, 256, NULL, &len);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Plain size = %lu\n", len);

	len = sizeof(plain);
	rc = p11->C_Decrypt(session, (CK_BYTE_PTR)p15_cryptogram, 256, plain, &len);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	bin2str(scr, sizeof(scr), plain, len);
	printf("Plain:\n%s\n", scr);

}



void testSessions(CK_FUNCTION_LIST_PTR p11, CK_SLOT_ID slotid)
{
	int rc;
	CK_SESSION_INFO sessioninfo;
	CK_SESSION_HANDLE session1, session2, session3;

	printf("Calling C_OpenSession ");
	rc = p11->C_OpenSession(slotid, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session1);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_OpenSession ");
	rc = p11->C_OpenSession(slotid, CKF_SERIAL_SESSION, NULL, NULL, &session2);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session1, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session2, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RO_PUBLIC_SESSION));

	printf("Calling C_CloseSession ");
	rc = p11->C_CloseSession(session2);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_CloseSession with wrong handle ");
	rc = p11->C_CloseSession(session2);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_SESSION_HANDLE_INVALID));

	printf("Calling C_CloseSession ");
	rc = p11->C_CloseSession(session1);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	// Sequence inspired by PKCS#11 example
	printf("Calling C_OpenSession ");
	rc = p11->C_OpenSession(slotid, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session1);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_OpenSession ");
	rc = p11->C_OpenSession(slotid, CKF_SERIAL_SESSION, NULL, NULL, &session2);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_Login(SO) ");
	rc = p11->C_Login(session1, CKU_SO, sopin, sopinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_SESSION_READ_ONLY_EXISTS));

	printf("Calling C_Login(SO) ");
	rc = p11->C_Login(session2, CKU_SO, sopin, sopinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_SESSION_READ_ONLY));

	printf("Calling C_Login(USER) ");
	rc = p11->C_Login(session1, CKU_USER, pin, pinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session1, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session2, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RO_USER_FUNCTIONS));

	printf("Calling C_OpenSession ");
	rc = p11->C_OpenSession(slotid, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session3);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session3, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

	printf("Calling C_CloseSession ");
	rc = p11->C_CloseSession(session3);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_Logout ");
	rc = p11->C_Logout(session1);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session1, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session2, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RO_PUBLIC_SESSION));

	printf("Calling C_CloseAllSessions ");
	rc = p11->C_CloseAllSessions(slotid);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
}



void testLogin(CK_FUNCTION_LIST_PTR p11, CK_SESSION_HANDLE session)
{
	int rc;
	CK_SESSION_INFO sessioninfo;
	CK_TOKEN_INFO tokeninfo;

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_Login User ");
	rc = p11->C_Login(session, CKU_USER, pin, pinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		exit(1);
	}

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

	printf("Calling C_Logout ");
	rc = p11->C_Logout(session);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_GetTokenInfo ");
	rc = p11->C_GetTokenInfo(sessioninfo.slotID, &tokeninfo);
	printf("Token flags %lx - %s\n", tokeninfo.flags, verdict((tokeninfo.flags & (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY|CKF_USER_PIN_LOCKED)) == 0));


	printf("Calling C_Login User ");
	rc = p11->C_Login(session, CKU_USER, wrongpin, pinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_PIN_INCORRECT));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_GetTokenInfo ");
	rc = p11->C_GetTokenInfo(sessioninfo.slotID, &tokeninfo);
	printf("Token flags %lx - %s\n", tokeninfo.flags, verdict((tokeninfo.flags & (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY|CKF_USER_PIN_LOCKED)) == CKF_USER_PIN_COUNT_LOW));


	printf("Calling C_Login User ");
	rc = p11->C_Login(session, CKU_USER, wrongpin, pinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_PIN_INCORRECT));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

	printf("Calling C_GetTokenInfo ");
	rc = p11->C_GetTokenInfo(sessioninfo.slotID, &tokeninfo);
	printf("Token flags %lx - %s\n", tokeninfo.flags, verdict((tokeninfo.flags & (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY|CKF_USER_PIN_LOCKED)) == (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY)));

	if (optTestPINBlock) {
		printf("Calling C_Login User ");
		rc = p11->C_Login(session, CKU_USER, wrongpin, pinlen);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_PIN_LOCKED));

		printf("Calling C_GetSessionInfo ");
		rc = p11->C_GetSessionInfo(session, &sessioninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

		printf("Calling C_GetTokenInfo ");
		rc = p11->C_GetTokenInfo(sessioninfo.slotID, &tokeninfo);
		printf("Token flags %lx - %s\n", tokeninfo.flags, verdict((tokeninfo.flags & (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY|CKF_USER_PIN_LOCKED)) == (CKF_USER_PIN_LOCKED)));
	}

	printf("Calling C_Login User ");
	rc = p11->C_Login(session, CKU_USER, pin, pinlen);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	printf("Calling C_GetSessionInfo ");
	rc = p11->C_GetSessionInfo(session, &sessioninfo);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
	printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

	printf("Calling C_GetTokenInfo ");
	rc = p11->C_GetTokenInfo(sessioninfo.slotID, &tokeninfo);
	printf("Token flags %lx - %s\n", tokeninfo.flags, verdict((tokeninfo.flags & (CKF_USER_PIN_COUNT_LOW|CKF_USER_PIN_FINAL_TRY|CKF_USER_PIN_LOCKED)) == 0));

	if (tokeninfo.flags & CKF_PROTECTED_AUTHENTICATION_PATH) {

		printf("Calling C_Logout ");
		rc = p11->C_Logout(session);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		printf("Testing CKF_PROTECTED_AUTHENTICATION_PATH - Please enter correct PIN on pin-pad\n");
		printf("Calling C_Login User");
		rc = p11->C_Login(session, CKU_USER, NULL, 0);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (rc != CKR_OK) {
			exit(1);
		}

		printf("Calling C_GetSessionInfo ");
		rc = p11->C_GetSessionInfo(session, &sessioninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

		printf("Calling C_Logout ");
		rc = p11->C_Logout(session);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));


		printf("Testing CKF_PROTECTED_AUTHENTICATION_PATH - Please enter wrong PIN on pin-pad\n");
		printf("Calling C_Login User ");
		rc = p11->C_Login(session, CKU_USER, NULL, 0);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_PIN_INCORRECT));

		printf("Calling C_GetSessionInfo ");
		rc = p11->C_GetSessionInfo(session, &sessioninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_PUBLIC_SESSION));

		/*
		 * Reset the PIN counter
		 */
		printf("Calling C_Login User ");
		rc = p11->C_Login(session, CKU_USER, pin, pinlen);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (rc != CKR_OK) {
			exit(1);
		}

		printf("Calling C_GetSessionInfo ");
		rc = p11->C_GetSessionInfo(session, &sessioninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		printf("Session state %lu - %s\n", sessioninfo.state, verdict(sessioninfo.state == CKS_RW_USER_FUNCTIONS));

	}
}


void testInsertRemove(CK_FUNCTION_LIST_PTR p11, CK_SLOT_ID slotid)
{
	CK_RV rc;
	CK_SLOT_INFO slotinfo;
	CK_TOKEN_INFO tokeninfo;
	char *inp = NULL;
	size_t inplen;
	int loop;

	for (loop = 0; loop < 2; loop++) {
		printf("Please remove card from slot %lu and press <ENTER>\n", slotid);
		getchar();

		printf("Calling C_GetSlotInfo for slot %lu ", slotid);
		rc = p11->C_GetSlotInfo(slotid, &slotinfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (slotinfo.flags & CKF_TOKEN_PRESENT) {
			printf("slotinfo.flags - Failed\n");
		}

		printf("Calling C_GetTokenInfo ");
		rc = p11->C_GetTokenInfo(slotid, &tokeninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_TOKEN_NOT_PRESENT));

		printf("Please insert card in slot %lu and press <ENTER>\n", slotid);
		getchar();

		printf("Calling C_GetSlotInfo for slot %lu ", slotid);
		rc = p11->C_GetSlotInfo(slotid, &slotinfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (!(slotinfo.flags & CKF_TOKEN_PRESENT)) {
			printf("slotinfo.flags - Failed\n");
		}

		printf("Calling C_GetTokenInfo ");
		rc = p11->C_GetTokenInfo(slotid, &tokeninfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (rc == CKR_OK) {
			printf("Token label: %s\n", p11string(tokeninfo.label, sizeof(tokeninfo.label)));
		}
	}
}



void usage()
{
	printf("sc-hsm-tool [--module <p11-file>] [--pin <user-pin>]\n");
	printf("  --test-insert-remove       Enable insert / remove test\n");
	printf("  --test-rsa-decryption      Enable RSA decryption test (requires matching cryptogram in testRSADecryption()\n");
	printf("  --test-pin-block           Enable PIN blocking test\n");
	printf("  --test-multithreading-only Perform multihreading tests only\n");
	printf("  --one-thread-per-token     Create a single thread per token\n");
	printf("  --max-parallel-threads     Number of cuncurrently running threads (default 10)\n");
	printf("  --thread-test-time         Time limit for thread tests in seconds (default 10 s)\n");
	exit(1);
}



void decodeArgs(int argc, char **argv)
{
	argv++;
	argc--;

	for ( ; argc--; argv++) {
		if (!strcmp(*argv, "--pin")) {
			if (argc < 0) {
				printf("Argument for --pin missing\n");
				exit(1);
			}
			pin = (CK_UTF8CHAR_PTR)*++argv;
			pinlen = strlen((char *)pin);
			argc--;
		} else if (!strcmp(*argv, "--module")) {
			if (argc < 0) {
				printf("Argument for --module missing\n");
				exit(1);
			}
			argv++;
			p11libname = *argv;
			argc--;
		} else if (!strcmp(*argv, "--test-insert-remove")) {
			optTestInsertRemove = 1;
		} else if (!strcmp(*argv, "--test-rsa-decryption")) {
			optTestRSADecryption = 1;
		} else if (!strcmp(*argv, "--test-pin-block")) {
			optTestPINBlock = 1;
		} else if (!strcmp(*argv, "--test-multithreading-only")) {
			optTestMultiOnly = 1;
		} else if (!strcmp(*argv, "--one-thread-per-token")) {
			optOneThreadPerToken = 1;
		} else if (!strcmp(*argv, "--max-parallel-threads")) {
			if (argc-- == 0)
				usage();
			optMaxParallelThreads = atoi(*++argv);
		} else if (!strcmp(*argv, "--thread-test-time")) {
			if (argc-- == 0)
				usage();
			optThreadTestTime = atoi(*++argv) * 1000;
		} else {
			printf("Unknown argument %s\n", *argv);
			usage();
		}
	}
}

void ctrlCHandler(int sig)
{
	requestClose = 256;
	signal(SIGINT, 0); /* disable handler */
	fprintf(stderr, "<CTRL>C received, no new thread will be created, please wait ...\n");
}

int main(int argc, char *argv[])
{
	int i;
	CK_RV rc;
	CK_ULONG slots;
	CK_SESSION_HANDLE session;
	CK_INFO info;
	CK_SLOT_ID_PTR slotlist;
	CK_SLOT_ID slotid;
	CK_SLOT_INFO slotinfo;
	CK_TOKEN_INFO tokeninfo;
	CK_ATTRIBUTE attr[6];
	CK_FUNCTION_LIST_PTR p11;
	LIB_HANDLE dlhandle;
	CK_RV (*C_GetFunctionList)(CK_FUNCTION_LIST_PTR_PTR);
	CK_C_INITIALIZE_ARGS initArgs;

	unsigned RSASignCount = 0;
	unsigned RSATokenCount = 0;
	long RSAElapsed = 0;

	unsigned ECDSASignCount = 0;
	unsigned ECDSATokenCount = 0;
	long ECDSAElapsed = 0;

#if defined(_WIN32) && defined(_DEBUG)
	atexit((void(*)(void))_CrtDumpMemoryLeaks);
	//_CrtSetBreakAlloc(494);
#endif

	decodeArgs(argc, argv);

	printf("PKCS11 unittest running.\n");

	dlhandle = dlopen(p11libname, RTLD_NOW);

	if (!dlhandle) {
		printf("dlopen failed with %s\n", dlerror());
		exit(1);
	}

	C_GetFunctionList = (CK_RV (*)(CK_FUNCTION_LIST_PTR_PTR))dlsym(dlhandle, "C_GetFunctionList");

	printf("Calling C_GetFunctionList ");

	(*C_GetFunctionList)(&p11);

	memset(&initArgs, 0, sizeof(initArgs));
	initArgs.flags = CKF_OS_LOCKING_OK;

	printf("Calling C_Initialize ");

	rc = p11->C_Initialize(&initArgs);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		exit(1);
	}

	printf("Calling C_GetInfo ");

	rc = p11->C_GetInfo(&info);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		exit(1);
	}

	printf("Calling C_GetSlotList ");

	rc = p11->C_GetSlotList(FALSE, NULL, &slots);

	if (rc != CKR_OK) {
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		exit(1);
	}

	slotlist = (CK_SLOT_ID_PTR)malloc(sizeof(CK_SLOT_ID) * slots);

	rc = p11->C_GetSlotList(FALSE, slotlist, &slots);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		exit(1);
	}

	for (i = 0; i < slots; i++) {
		slotid = slotlist[i];

		if (optTestInsertRemove)
			testInsertRemove(p11, slotid);

		printf("Calling C_GetSlotInfo for slot %lu ", slotid);
		rc = p11->C_GetSlotInfo(slotid, &slotinfo);
		printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

		if (rc != CKR_OK) {
			printf("Error getting slot information from cryptoki. slotid = %lu, rc = %lu = %s\n", slotid, rc, CKR_Name(rc));
			free(slotlist);
			exit(1);
		}

		printf("Slot manufacturer: %s\n", p11string(slotinfo.manufacturerID, sizeof(slotinfo.manufacturerID)));
		printf("Slot ID : %ld Description: %s\n", slotid, p11string(slotinfo.slotDescription, sizeof(slotinfo.slotDescription)));
		printf("Slot flags: %x\n", (int)slotinfo.flags);

		printf("Calling C_GetTokenInfo ");

		rc = p11->C_GetTokenInfo(slotid, &tokeninfo);
		printf("- %s : %s\n", CKR_Name(rc), rc == CKR_OK ? "Passed" : rc == CKR_TOKEN_NOT_PRESENT ? "No token" : "Failed");

		switch (rc) {
		case CKR_OK:
		case CKR_TOKEN_NOT_PRESENT:
		case CKR_DEVICE_REMOVED:
			break;
		default:
			printf("Error getting token information from cryptoki. slotid = %lu, rc = %lu = %s\n", slotid, rc, CKR_Name(rc));
			free(slotlist);
			exit(1);
		}

		if (rc == CKR_OK) {
			printf("Token label: %s\n", p11string(tokeninfo.label, sizeof(tokeninfo.label)));
			printf("Token flags: %lx\n", tokeninfo.flags);

			if (optTestMultiOnly)
				continue;

			testSessions(p11, slotid);

			rc = p11->C_OpenSession(slotid, CKF_RW_SESSION | CKF_SERIAL_SESSION, NULL, NULL, &session);
			printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

			if (rc != CKR_OK) {
				exit(1);
			}

			testLogin(p11, session);

			// List all objects
			memset(attr, 0, sizeof(attr));
			listObjects(p11, session, attr, 0);

			testSigning(p11, slotid, CKK_RSA, 0);

			//	Test requires valid crypto matching card used for testing
			if (optTestRSADecryption)
				testRSADecryption(p11, session);

			testSigning(p11, slotid, CKK_ECDSA, 0);

			printf("Calling C_CloseSession\n");
			rc = p11->C_CloseSession(session);
			printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));
		}
	}

	signal(SIGINT, ctrlCHandler);

	testSigningMultiThreaded(p11, CKK_RSA, &RSASignCount, &RSATokenCount, &RSAElapsed);

	testSigningMultiThreaded(p11, CKK_ECDSA, &ECDSASignCount, &ECDSATokenCount, &ECDSAElapsed);

	printf("Calling C_Finalize ");
	rc = p11->C_Finalize(NULL);
	printf("- %s : %s\n", CKR_Name(rc), verdict(rc == CKR_OK));

	if (rc != CKR_OK) {
		exit(1);
	}

	dlclose(dlhandle);
	free(slotlist);

	printf("Unit test finished.\n");
	printf("%d tests performed.\n", testsCompleted);
	printf("%d tests failed.\n", testsFailed);

	if (RSASignCount > 0) {
		printf("%u RSA signatures with %u tokens in %ld.%ld s (%.0ld ms/op)\n",
			RSASignCount, RSATokenCount, RSAElapsed / 1000, RSAElapsed % 1000, RSAElapsed / RSASignCount);
	}
	if (ECDSASignCount > 0) {
		printf("%u ECDSA signatures with %u tokens in %ld.%ld s (%ld ms/op)\n",
			ECDSASignCount, ECDSATokenCount, ECDSAElapsed / 1000, ECDSAElapsed % 1000, ECDSAElapsed / ECDSASignCount);
	}
	if (tooManyKeysReported > 0) {
		printf("%u curruption detected (too many keys reported)\n", tooManyKeysReported);
	}

	exit(testsFailed ? 1 : 0);
}
