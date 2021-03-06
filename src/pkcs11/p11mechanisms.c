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
 * @file    p11mechanisms.c
 * @author  Frank Thater, Andreas Schwier
 * @brief   Crypto mechanisms at the PKCS#11 interface
 */

#include <pkcs11/p11generic.h>
#include <pkcs11/session.h>
#include <pkcs11/slot.h>
#include <pkcs11/slotpool.h>
#include <pkcs11/token.h>
#include <pkcs11/debug.h>


extern struct p11Context_t *context;



/*  C_EncryptInit initializes an encryption operation. */
CK_DECLARE_FUNCTION(CK_RV, C_EncryptInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle != CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_ACTIVE, "Operation is already active");
	}

	rv = findSlotObject(slot, hKey, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_EncryptInit != NULL) {
		rv = object->C_EncryptInit(object, pMechanism);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (!rv) {
		session->activeObjectHandle = object->handle;
		rv = CKR_OK;
	}

	FUNC_RETURNS(rv);
}



/*  C_Encrypt encrypts single-part data. */
CK_DECLARE_FUNCTION(CK_RV, C_Encrypt)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pData,
		CK_ULONG ulDataLen,
		CK_BYTE_PTR pEncryptedData,
		CK_ULONG_PTR pulEncryptedDataLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_Encrypt != NULL) {
		rv = object->C_Encrypt(object, session->activeMechanism, pData, ulDataLen, pEncryptedData, pulEncryptedDataLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	FUNC_RETURNS(rv);
}



/*  C_EncryptUpdate continues a multiple-part encryption operation,
    processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_EncryptUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG_PTR pulEncryptedPartLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_EncryptUpdate != NULL) {
		rv = object->C_EncryptUpdate(object, session->activeMechanism, pPart, ulPartLen, pEncryptedPart, pulEncryptedPartLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	FUNC_RETURNS(rv);
}



/*  C_EncryptFinal finishes a multiple-part encryption operation. */
CK_DECLARE_FUNCTION(CK_RV, C_EncryptFinal)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pLastEncryptedPart,
		CK_ULONG_PTR pulLastEncryptedPartLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_EncryptFinal != NULL) {
		rv = object->C_EncryptFinal(object, session->activeMechanism, pLastEncryptedPart, pulLastEncryptedPartLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (!rv) {
		session->activeObjectHandle = CK_INVALID_HANDLE;
		rv = CKR_OK;
	}

	FUNC_RETURNS(rv);
}



/*  C_DecryptInit initializes a decryption operation. */
CK_DECLARE_FUNCTION(CK_RV, C_DecryptInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle != CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_ACTIVE, "Operation is already active");
	}

	rv = findSlotObject(slot, hKey, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_DecryptInit != NULL) {
		rv = object->C_DecryptInit(object, pMechanism);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (!rv) {
		session->activeObjectHandle = object->handle;
		session->activeMechanism = pMechanism->mechanism;
		rv = CKR_OK;
	}

	FUNC_RETURNS(rv);
}



/*  C_Decrypt decrypts encrypted data in a single part. */
CK_DECLARE_FUNCTION(CK_RV, C_Decrypt)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pEncryptedData,
		CK_ULONG ulEncryptedDataLen,
		CK_BYTE_PTR pData,
		CK_ULONG_PTR pulDataLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (pData != NULL) {
		session->activeObjectHandle = CK_INVALID_HANDLE;
	}

	if (object->C_Decrypt != NULL) {
		rv = object->C_Decrypt(object, session->activeMechanism, pEncryptedData, ulEncryptedDataLen, pData, pulDataLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	FUNC_RETURNS(rv);
}



/*  C_DecryptUpdate continues a multiple-part decryption operation,
    processing another encrypted data part. */
CK_DECLARE_FUNCTION(CK_RV, C_DecryptUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG ulEncryptedPartLen,
		CK_BYTE_PTR pPart,
		CK_ULONG_PTR pulPartLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_DecryptUpdate != NULL) {
		rv = object->C_DecryptUpdate(object, session->activeMechanism, pEncryptedPart, ulEncryptedPartLen, pPart, pulPartLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	FUNC_RETURNS(rv);
}



/*  C_DecryptFinal finishes a multiple-part decryption operation. */
CK_DECLARE_FUNCTION(CK_RV, C_DecryptFinal)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pLastPart,
		CK_ULONG_PTR pulLastPartLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_DecryptFinal != NULL) {
		rv = object->C_DecryptFinal(object, session->activeMechanism, pLastPart, pulLastPartLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (!rv) {
		session->activeObjectHandle = CK_INVALID_HANDLE;
		rv = CKR_OK;
	}

	FUNC_RETURNS(rv);
}



/*  C_DigestInit initializes a message-digesting operation. */
CK_DECLARE_FUNCTION(CK_RV, C_DigestInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_Digest digests data in a single part. */
CK_DECLARE_FUNCTION(CK_RV, C_Digest)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pData,
		CK_ULONG ulDataLen,
		CK_BYTE_PTR pDigest,
		CK_ULONG_PTR pulDigestLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DigestUpdate continues a multiple-part message-digesting operation,
    processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_DigestUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DigestKey continues a multiple-part message-digesting operation by
    digesting the value of a secret key. */
CK_DECLARE_FUNCTION(CK_RV, C_DigestKey)(
		CK_SESSION_HANDLE hSession,
		CK_OBJECT_HANDLE hKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DigestFinal finishes a multiple-part message-digesting operation. */
CK_DECLARE_FUNCTION(CK_RV, C_DigestFinal)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pDigest,
		CK_ULONG_PTR pulDigestLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_SignInit initializes a signature operation,
    here the signature is an appendix to the data. */
CK_DECLARE_FUNCTION(CK_RV, C_SignInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle != CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_ACTIVE, "Operation is already active");
	}

	rv = findSlotObject(slot, hKey, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_SignInit != NULL) {
		rv = object->C_SignInit(object, pMechanism);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (!rv) {
		session->activeObjectHandle = object->handle;
		session->activeMechanism = pMechanism->mechanism;
		rv = CKR_OK;
	}

	FUNC_RETURNS(rv);
}



/*  C_Sign signs data in a single part, where the signature is an appendix to the data. */
CK_DECLARE_FUNCTION(CK_RV, C_Sign)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pData,
		CK_ULONG ulDataLen,
		CK_BYTE_PTR pSignature,
		CK_ULONG_PTR pulSignatureLen
)
{
	int rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (pSignature != NULL) {
		session->activeObjectHandle = CK_INVALID_HANDLE;
	}

	if (object->C_Sign != NULL) {
		rv = object->C_Sign(object, session->activeMechanism, pData, ulDataLen, pSignature, pulSignatureLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	FUNC_RETURNS(rv);
}



/*  C_SignUpdate continues a multiple-part signature operation,
    processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_SignUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen
)
{
	CK_RV rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (object->C_SignUpdate != NULL) {
		rv = object->C_SignUpdate(object, session->activeMechanism, pPart, ulPartLen);
	} else {
		rv = appendToCryptoBuffer(session, pPart, ulPartLen);
	}

	FUNC_RETURNS(rv);
}



/*  C_SignFinal finishes a multiple-part signature operation. */
CK_DECLARE_FUNCTION(CK_RV, C_SignFinal)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pSignature,
		CK_ULONG_PTR pulSignatureLen
)
{
	CK_RV rv;
	struct p11Object_t *object;
	struct p11Session_t *session;
	struct p11Slot_t *slot;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_FIND_SESSION_AND_LOCK_SLOT(hSession, &session, &slot);

	if (session->activeObjectHandle == CK_INVALID_HANDLE) {
		FUNC_FAILS(CKR_OPERATION_NOT_INITIALIZED, "Operation not initialized");
	}

	rv = findSlotObject(slot, session->activeObjectHandle, &object, FALSE);

	if (rv != CKR_OK) {
		FUNC_RETURNS(rv);
	}

	if (pSignature != NULL) {
		session->activeObjectHandle = CK_INVALID_HANDLE;
	}

	if (object->C_SignFinal != NULL) {
		rv = object->C_SignFinal(object, session->activeMechanism, pSignature, pulSignatureLen);
	} else if (object->C_Sign != NULL) {
		rv = object->C_Sign(object, session->activeMechanism, session->cryptoBuffer, session->cryptoBufferSize, pSignature, pulSignatureLen);
	} else {
		FUNC_FAILS(CKR_FUNCTION_NOT_SUPPORTED, "Operation not supported by token");
	}

	if (pSignature != NULL) {
		clearCryptoBuffer(session);
	}

	FUNC_RETURNS(rv);
}



/*  C_SignRecoverInit initializes a signature operation, where the data
    can be recovered from the signature. */
CK_DECLARE_FUNCTION(CK_RV, C_SignRecoverInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_SignRecover signs data in a single operation, where the data can be
    recovered from the signature. */
CK_DECLARE_FUNCTION(CK_RV, C_SignRecover)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pData,
		CK_ULONG ulDataLen,
		CK_BYTE_PTR pSignature,
		CK_ULONG_PTR pulSignatureLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_VerifyInit initializes a verification operation, where the signature is
    an appendix to the data. */
CK_DECLARE_FUNCTION(CK_RV, C_VerifyInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_Verify verifies a signature in a single-part operation, where the signature
    is an appendix to the data. */
CK_DECLARE_FUNCTION(CK_RV, C_Verify)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pData,
		CK_ULONG ulDataLen,
		CK_BYTE_PTR pSignature,
		CK_ULONG ulSignatureLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_VerifyUpdate continues a multiple-part verification operation,
    processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_VerifyUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_VerifyFinal finishes a multiple-part verification operation,
    checking the signature. */
CK_DECLARE_FUNCTION(CK_RV, C_VerifyFinal)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pSignature,
		CK_ULONG ulSignatureLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_VerifyRecoverInit initializes a signature verification operation,
    where the data is recovered from the signature. */
CK_DECLARE_FUNCTION(CK_RV, C_VerifyRecoverInit)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_VerifyRecover verifies a signature in a single-part operation,
    where the data is recovered from the signature. */
CK_DECLARE_FUNCTION(CK_RV, C_VerifyRecover)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pSignature,
		CK_ULONG ulSignatureLen,
		CK_BYTE_PTR pData,
		CK_ULONG_PTR pulDataLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DigestEncryptUpdate continues multiple-part digest and encryption
    operations, processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_DigestEncryptUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG_PTR pulEncryptedPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DecryptDigestUpdate continues a multiple-part combined decryption and
    digest operation, processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_DecryptDigestUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG ulEncryptedPartLen,
		CK_BYTE_PTR pPart,
		CK_ULONG_PTR pulPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_SignEncryptUpdate continues a multiple-part combined signature and
    encryption operation, processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_SignEncryptUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pPart,
		CK_ULONG ulPartLen,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG_PTR pulEncryptedPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DecryptVerifyUpdate continues a multiple-part combined decryption and verification
    operation, processing another data part. */
CK_DECLARE_FUNCTION(CK_RV, C_DecryptVerifyUpdate)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pEncryptedPart,
		CK_ULONG ulEncryptedPartLen,
		CK_BYTE_PTR pPart,
		CK_ULONG_PTR pulPartLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_GenerateKey generates a secret key or set of domain parameters,
    creating a new object. */
CK_DECLARE_FUNCTION(CK_RV, C_GenerateKey)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_GenerateKeyPair generates a public/private key pair, creating new key objects. */
CK_DECLARE_FUNCTION(CK_RV, C_GenerateKeyPair)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_WrapKey wraps (i.e., encrypts) a private or secret key. */
CK_DECLARE_FUNCTION(CK_RV, C_WrapKey)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hWrappingKey,
		CK_OBJECT_HANDLE hKey,
		CK_BYTE_PTR pWrappedKey,
		CK_ULONG_PTR pulWrappedKeyLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_UnwrapKey unwraps (i.e. decrypts) a wrapped key, creating a new private key
    or secret key object. */
CK_DECLARE_FUNCTION(CK_RV, C_UnwrapKey)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hUnwrappingKey,
		CK_BYTE_PTR pWrappedKey,
		CK_ULONG ulWrappedKeyLen,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulAttributeCount,
		CK_OBJECT_HANDLE_PTR phKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_DeriveKey derives a key from a base key, creating a new key object. */
CK_DECLARE_FUNCTION(CK_RV, C_DeriveKey)(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulAttributeCount,
		CK_OBJECT_HANDLE_PTR phKey
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_SeedRandom mixes additional seed material into the token�s random number generator. */
CK_DECLARE_FUNCTION(CK_RV, C_SeedRandom)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pSeed,
		CK_ULONG ulSeedLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_GenerateRandom generates random or pseudo-random data. */
CK_DECLARE_FUNCTION(CK_RV, C_GenerateRandom)(
		CK_SESSION_HANDLE hSession,
		CK_BYTE_PTR pRandomData,
		CK_ULONG ulRandomLen
)
{
	CK_RV rv = CKR_FUNCTION_NOT_SUPPORTED;

	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(rv);
}



/*  C_GetFunctionStatus obtained the status of a function
    running in parallel with an application. Now legacy! */
CK_DECLARE_FUNCTION(CK_RV, C_GetFunctionStatus)(
		CK_SESSION_HANDLE hSession
)
{
	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(CKR_FUNCTION_NOT_PARALLEL);
}


/*  C_CancelFunction cancelled a function running in parallel
    with an application. Now legacy! */
CK_DECLARE_FUNCTION(CK_RV, C_CancelFunction)(
		CK_SESSION_HANDLE hSession
)
{
	FUNC_CALLED();

	if (context == NULL) {
		FUNC_FAILS(CKR_CRYPTOKI_NOT_INITIALIZED, "C_Initialize not called");
	}

	FUNC_RETURNS(CKR_FUNCTION_NOT_PARALLEL);
}
