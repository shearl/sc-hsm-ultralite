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
 * @file    session.h
 * @author  Frank Thater, Andreas Schwier
 * @brief   Data types and functions for session management
 */

#ifndef ___SESSION_H_INC___
#define ___SESSION_H_INC___

#include <pkcs11/p11generic.h>
#include <pkcs11/cryptoki.h>
#include <pkcs11/object.h>


struct p11ObjectSearch_t
{
	int objectCount;
	int objectCollected; /* so far */
	struct p11Object_t *searchList;
};


/**
 * Internal structure to store information about specific session.
 *
 */

struct p11Session_t
{
	CK_FLAGS flags;                     /**< The flags of this session                 */
	CK_SLOT_ID slotID;                  /**< The the slot for this session             */
	CK_SESSION_HANDLE handle;           /**< The handle of the session                 */
	unsigned queuing;                   /**< Used to preventing session deletion       */
	int activeObjectHandle;             /**< active object or CK_INVALID_HANDLE        */
	CK_MECHANISM_TYPE activeMechanism;  /**< The currently active mechanism            */
	CK_BYTE_PTR cryptoBuffer;           /**< Buffer storing intermediate results       */
	CK_ULONG cryptoBufferSize;          /**< Current content of crypto buffer          */
	CK_ULONG cryptoBufferMax;           /**< Current size of crypto buffer             */
	struct p11ObjectSearch_t searchObj; /**< Store the result of a search operation    */
	CK_LONG nextSessionObjHandle;       /**< Value of next assigned object handle      */
	int objectCount;                    /**< The number of objects in this session     */
	struct p11Object_t *objectList;     /**< Pointer to first object in pool           */
	struct p11Session_t *next;          /**< Pointer to next active session, NULL else */
};

/* function prototypes */

void initSessionPool(struct p11SessionPool_t *pool);
void terminateSessionPool(struct p11SessionPool_t *pool);
void freeSession(struct p11Session_t *session);
void safeAddSession(struct p11SessionPool_t *pool, struct p11Session_t *session);
int safeFindSessionAndLockSlot(struct p11SessionPool_t *sessionPool, struct p11SlotPool_t *slotPool,
	CK_SESSION_HANDLE handle, struct p11Session_t **ppSession, struct p11Slot_t **ppSlot);
int safeFindFirstSessionBySlotID(struct p11SessionPool_t *pool, CK_SLOT_ID slotID, CK_SESSION_HANDLE *phSession);
CK_STATE getSessionState(struct p11Session_t *session, struct p11Slot_t *slot);
void addSessionObject(struct p11Session_t *session, struct p11Object_t *object);
int findSessionObject(struct p11Session_t *session, CK_OBJECT_HANDLE handle, struct p11Object_t **object);
int removeSessionObject(struct p11Session_t *session, CK_OBJECT_HANDLE handle);
int addObjectToSearchList(struct p11Session_t *session, struct p11Object_t *object);
void clearSearchList(struct p11Session_t *session);
int appendToCryptoBuffer(struct p11Session_t *session, CK_BYTE_PTR data, CK_ULONG length);
void clearCryptoBuffer(struct p11Session_t *session);

#endif /* ___SESSION_H_INC___ */
