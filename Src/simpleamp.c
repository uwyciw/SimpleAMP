/**
  ******************************************************************************
  * @file simpleamp.c
  * @author lx
  * @version
  * @date 2026-03-14
  * @brief
 =============================================================================
                     #####  #####
 =============================================================================

  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "simpleamp.h"
#include <stdatomic.h>

/**
 * @brief
 * @note
 * @param
 * @retval
 */
void SAMPIint(SAMP_HANDLE_T * pHandle, unsigned int * pAvailRing, unsigned int * pUsedRing, SAMP_MAIL_T * pMail, unsigned int num)
{
    if (num == 0) {
        return;
    }

    pHandle->num = num;
    pHandle->availIndex = 0u;
    pHandle->usedIndex = 0u;
    pHandle->pAvailRing = pAvailRing;
    pHandle->pUsedRing = pUsedRing;
    pHandle->pMail = pMail;
    for (unsigned int i = 0u; i < num; i++) {
        pMail[i].seq = i;
        pMail[i].isIdle = true;
    }
}

/**
 * @brief
 * @note
 * @param
 * @retval
 */
SAMP_MAIL_T * SAMPGetIdleMail(SAMP_HANDLE_T * pHandle)
{
    SAMP_MAIL_T * pMail = NULL;

    for (unsigned int i = 0u; i < pHandle->num; i++) {
        if (pHandle->pMail[i].isIdle == true) {
            pMail = &(pHandle->pMail[i]);
        }
    }

    return pMail;
}

/**
 * @brief
 * @note
 * @param
 * @retval
 */
void SAMPSendMail(SAMP_HANDLE_T * pHandle, SAMP_MAIL_T * pMail)
{
    unsigned int index = (pHandle->availIndex) % (pHandle->num);
    pHandle->pAvailRing[index] = pMail->seq;
    pMail->isIdle = false;
    atomic_thread_fence(memory_order_seq_cst);
    pHandle->availIndex = pHandle->availIndex + 1u;
}

/**
 * @brief
 * @note
 * @param
 * @retval
 */
unsigned int SAMPFreeMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastUsedIndex)
{
    unsigned int counter;
    unsigned int index = (*pLastUsedIndex) % (pHandle->num);
    unsigned int * usedRing = pHandle->pUsedRing;
    SAMP_MAIL_T * mailArray = pHandle->pMail;

    atomic_thread_fence(memory_order_seq_cst);

    if (pHandle->usedIndex == (*pLastUsedIndex))
        return 0u;

    counter = pHandle->usedIndex - (*pLastUsedIndex);
    for (unsigned int i = 0u; i < counter; i++) {
        mailArray[usedRing[index + i]].isIdle = true;
    }
    *pLastUsedIndex = (*pLastUsedIndex) + counter;

    return counter;
}

/**
 * @brief
 * @note
 * @param
 * @retval
 */
SAMP_MAIL_T * SAMPReceiveMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastAvailIndex)
{
    SAMP_MAIL_T * mail = NULL;

    atomic_thread_fence(memory_order_seq_cst);

    if (pHandle->availIndex == (*pLastAvailIndex))
        return mail;

    mail = pHandle->pMail + (pHandle->pAvailRing[(*pLastAvailIndex) % (pHandle->num)]);
    *pLastAvailIndex = *pLastAvailIndex + 1u;
    
    return mail;
}
