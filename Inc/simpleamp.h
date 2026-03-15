/**
  ******************************************************************************
  * @file simpleamp.h
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

#ifndef _SIMPLEAMP_H_
#define _SIMPLEAMP_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

typedef enum {
 SAMP_MAIL_TYPE_RO = 0u,
 SAMP_MAIL_TYPE_W,
} SAMP_MAIL_TYPE_T;

/**
 * @brief
 */
typedef struct {
    SAMP_MAIL_TYPE_T type;
    bool isIdle;
    unsigned int seq;
    unsigned int length;
    void * address;
} SAMP_MAIL_T;

/**
 * @brief
 */
typedef struct {
    unsigned int num;
    unsigned int availIndex;
    unsigned int usedIndex;
    unsigned int * pAvailRing;
    unsigned int * pUsedRing;
    SAMP_MAIL_T * pMail;
} SAMP_HANDLE_T;

/**
 * @brief
 * @note
 * @param
 * @retval
 */
void SAMPIint(SAMP_HANDLE_T * pHandle, unsigned int * pAvailRing, unsigned int * pUsedRing, SAMP_MAIL_T *pMail,unsigned int num);

/**
 * @brief
 * @note
 * @param
 * @retval
 */
SAMP_MAIL_T * SAMPGetIdleMail(SAMP_HANDLE_T * pHandle);

/**
 * @brief
 * @note
 * @param
 * @retval
 */
void SAMPSendMail(SAMP_HANDLE_T * pHandle, SAMP_MAIL_T * pMail);

/**
 * @brief
 * @note
 * @param
 * @retval
 */
unsigned int SAMPFreeMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastUsedIndex);

/**
 * @brief
 * @note
 * @param
 * @retval
 */
SAMP_MAIL_T * SAMPReceiveMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastAvailIndex);

#endif // _SIMPLEAMP_H_
