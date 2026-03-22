/**
 ******************************************************************************
 * @file simpleamp.c
 * @author lx
 * @version
 * @date 2026-03-14
 * @brief SimpleAMP - 面向异构多核场景的轻量级通信框架实现文件
 ******************************************************************************
 * @attention
 * 该框架专为异构多核系统设计，通过内存共享方式实现高效核间通信
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "simpleamp.h"
#include <stdatomic.h>

/**
 * @brief SimpleAMP初始化函数实现
 * 初始化邮箱队列句柄及相关数据结构，设置初始状态
 * @note 此函数必须在使用其他API之前调用，以正确设置邮箱队列
 * @param pHandle 指向邮箱句柄的指针
 * @param pAvailRing 指向可用环数组的指针
 * @param pUsedRing 指向已用环数组的指针
 * @param pMail 指向邮箱数组的指针
 * @param num 邮箱数量
 * @retval 无
 */
void SAMPInit(SAMP_HANDLE_T * pHandle, unsigned int * pAvailRing, unsigned int * pUsedRing, SAMP_MAIL_T * pMail, unsigned int num)
{
    // 如果邮箱数量为0，则直接返回
    if (num == 0) {
        return;
    }

    // 初始化句柄中的各项参数
    pHandle->num = num;               // 设置邮箱总数
    pHandle->availIndex = 0u;         // 初始化可用索引为0
    pHandle->usedIndex = 0u;          // 初始化已用索引为0
    pHandle->pAvailRing = pAvailRing; // 设置可用环数组指针
    pHandle->pUsedRing = pUsedRing;   // 设置已用环数组指针
    pHandle->pMail = pMail;           // 设置邮箱数组指针

    // 遍历所有邮箱，初始化其状态
    for (unsigned int i = 0u; i < num; i++) {
        pMail[i].seq = i;       // 设置邮箱序列号
        pMail[i].isIdle = true; // 标记邮箱为空闲状态
    }
}

/**
 * @brief 获取空闲邮箱函数实现
 * 在邮箱队列中查找并返回一个空闲状态的邮箱
 * @note 此函数遍历邮箱数组查找isIdle为true的邮箱，如果找到多个空闲邮箱则返回最后一个
 * @param pHandle 指向邮箱句柄的指针
 * @retval 成功时返回空闲邮箱的指针，无空闲邮箱时返回NULL
 */
SAMP_MAIL_T * SAMPGetIdleMail(SAMP_HANDLE_T * pHandle)
{
    SAMP_MAIL_T * pMail = NULL;

    // 遍历所有邮箱，查找空闲状态的邮箱
    for (unsigned int i = 0u; i < pHandle->num; i++) {
        if (pHandle->pMail[i].isIdle == true) {
            // 找到空闲邮箱，保存其指针
            pMail = &(pHandle->pMail[i]);
        }
    }

    return pMail;
}

/**
 * @brief 发送邮箱函数实现
 * 将指定的邮箱加入到可用环中，通知对端核心有新数据待处理
 * @note 此函数将邮箱标记为非空闲，并更新可用索引，确保内存操作顺序
 * @param pHandle 指向邮箱句柄的指针
 * @param pMail 指向待发送邮箱的指针
 * @retval 无
 */
void SAMPSendMail(SAMP_HANDLE_T * pHandle, SAMP_MAIL_T * pMail)
{
    // 计算在可用环中的索引位置（循环使用）
    unsigned int index = (pHandle->availIndex) % (pHandle->num);

    // 将邮箱的序列号放入可用环中，通知对端核心
    pHandle->pAvailRing[index] = pMail->seq;

    // 将邮箱状态标记为非空闲（忙碌）
    pMail->isIdle = false;

    // 插入内存屏障，确保前面的操作在后续操作之前完成
    atomic_thread_fence(memory_order_seq_cst);

    // 更新可用索引，使对端核心可以发现新的可用邮箱
    pHandle->availIndex = pHandle->availIndex + 1u;
}

/**
 * @brief 释放已处理邮箱函数实现
 * 释放已完成处理的邮箱，将其状态标记为空闲，以便重新使用
 * @note 此函数检查已用环，将对应的邮箱标记为isIdle=true，返回释放的邮箱数量
 * @param pHandle 指向邮箱句柄的指针
 * @param pLastUsedIndex 指向上次处理完成索引的指针
 * @retval 返回被释放的邮箱数量
 */
unsigned int SAMPFreeUsedMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastUsedIndex)
{
    unsigned int counter;                         // 计数器，记录需要释放的邮箱数量
    unsigned int * usedRing = pHandle->pUsedRing; // 指向已用环的本地指针
    SAMP_MAIL_T * mailArray = pHandle->pMail;     // 指向邮箱数组的本地指针

    // 插入内存屏障，确保内存访问顺序正确
    atomic_thread_fence(memory_order_seq_cst);

    // 如果当前已用索引与上次处理完成的索引相同，则没有新的邮箱需要释放
    if (pHandle->usedIndex == (*pLastUsedIndex))
        return 0u;

    // 计算自上次处理以来新增的已处理邮箱数量
    counter = pHandle->usedIndex - (*pLastUsedIndex);

    // 遍历这些邮箱并将它们标记为空闲状态
    for (unsigned int i = 0u; i < counter; i++) {
        // 根据已用环中的序列号找到对应的邮箱并标记为空闲
        mailArray[usedRing[(*pLastUsedIndex) + i] % (pHandle->num)].isIdle = true;
    }

    // 更新上次处理完成的索引
    *pLastUsedIndex = (*pLastUsedIndex) + counter;

    // 返回释放的邮箱数量
    return counter;
}

/**
 * @brief 轮询接收邮箱函数实现
 * 从可用环中检查并获取待处理的邮箱，但不更新索引，仅用于检查是否有新数据
 * @note 此函数检查可用环，获取新的邮箱数据，但不改变内部状态，需配合SAMPTabUsedMail使用
 * @param pHandle 指向邮箱句柄的指针
 * @param lastAvailIndex 上次处理完成的索引
 * @retval 成功时返回接收到的邮箱指针，无新邮箱时返回NULL
 */
SAMP_MAIL_T * SAMPPollMail(SAMP_HANDLE_T * pHandle, unsigned int lastAvailIndex)
{
    SAMP_MAIL_T * mail; // 用于存储接收到的邮箱指针

    // 插入内存屏障，确保内存访问顺序正确
    atomic_thread_fence(memory_order_seq_cst);
    // 如果当前可用索引与上次接收完成的索引相同，则没有新的邮箱
    if (pHandle->availIndex == lastAvailIndex)
        return NULL;

    // 根据可用环中的序列号获取对应邮箱的指针
    mail = pHandle->pMail + (pHandle->pAvailRing[lastAvailIndex % (pHandle->num)]);

    // 返回接收到的邮箱指针
    return mail;
}

bool SAMPTabUsedMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastAvailIndex)
{
    SAMP_MAIL_T * mail; 
    unsigned int usedIndex;

    atomic_thread_fence(memory_order_seq_cst);
    if (pHandle->availIndex == (*pLastAvailIndex))
        return false;

    mail = pHandle->pMail + (pHandle->pAvailRing[(*pLastAvailIndex) % (pHandle->num)]);
    usedIndex = (pHandle->usedIndex) % (pHandle->num);

    pHandle->pUsedRing[usedIndex] = mail->seq;
    atomic_thread_fence(memory_order_seq_cst);
    pHandle->usedIndex = pHandle->usedIndex + 1u;

    *pLastAvailIndex = *pLastAvailIndex + 1u;

    return true;
}