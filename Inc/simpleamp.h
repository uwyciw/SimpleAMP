/**
 ******************************************************************************
 * @file simpleamp.h
 * @author lx
 * @version
 * @date 2026-03-14
 * @brief SimpleAMP - 面向异构多核场景的轻量级通信框架头文件
 ******************************************************************************
 * @attention
 * 该框架专为异构多核系统设计，支持不同核心间的高效数据传输
 ******************************************************************************
 */

#ifndef _SIMPLEAMP_H_
#define _SIMPLEAMP_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SAMP_MAIL_TYPE_RO = 0u, /*!< 邮件指向的内存区域，接收方只可以读取 */
    SAMP_MAIL_TYPE_W,       /*!< 邮件指向的内存区域，接收方可以写入 */
} SAMP_MAIL_TYPE_T;

/**
 * @brief 邮箱数据结构 - 表示一个可传输的数据单元
 * 用于在不同核心间传递数据
 */
typedef struct {
    SAMP_MAIL_TYPE_T type; /*!< 邮箱类型：指示当前邮件所指向的内存，接收方是否允许修改 */
    bool isIdle;           /*!< 空闲状态标志：true表示空闲，false表示已被使用 */
    unsigned int seq;      /*!< 序列号：用于标识邮箱的唯一序号 */
    unsigned int length;   /*!< 数据长度：要传输的数据字节数 */
    void * address;        /*!< 数据地址：指向实际数据的指针 */
} SAMP_MAIL_T;

/**
 * @brief SimpleAMP句柄结构体 - 管理邮箱队列的核心数据结构
 * 包含可用环和已用环来管理邮箱
 */
typedef struct {
    unsigned int num;          /*!< 邮箱总数：队列中邮箱的数量 */
    unsigned int availIndex;   /*!< 可用索引：指向下一个可用邮箱的位置 */
    unsigned int usedIndex;    /*!< 已用索引：指向下一个已处理邮箱的位置 */
    unsigned int * pAvailRing; /*!< 可用环数组：记录可用邮箱序列号的缓冲区 */
    unsigned int * pUsedRing;  /*!< 已用环数组：记录已处理邮箱序列号的缓冲区 */
    SAMP_MAIL_T * pMail;       /*!< 邮箱数组：实际的邮箱对象集合 */
} SAMP_HANDLE_T;

/**
 * @brief SimpleAMP初始化函数
 * 初始化邮箱队列句柄及相关数据结构
 * @note 此函数必须在使用其他API之前调用，以正确设置邮箱队列
 * @param pHandle 指向邮箱句柄的指针
 * @param pAvailRing 指向可用环数组的指针
 * @param pUsedRing 指向已用环数组的指针
 * @param pMail 指向邮箱数组的指针
 * @param num 邮箱数量
 * @retval 无
 */
void SAMPIint(SAMP_HANDLE_T * pHandle, unsigned int * pAvailRing, unsigned int * pUsedRing, SAMP_MAIL_T * pMail, unsigned int num);

/**
 * @brief
 * @note
 * @param
 * @retval
 */
SAMP_MAIL_T * SAMPGetIdleMail(SAMP_HANDLE_T * pHandle);

/**
 * @brief 发送邮箱函数
 * 将指定的邮箱加入到可用环中，通知对端核心有新数据待处理
 * @note 此函数将邮箱标记为非空闲，并更新可用索引，类似VirtIO的buffer提交
 * @param pHandle 指向邮箱句柄的指针
 * @param pMail 指向待发送邮箱的指针
 * @retval 无
 */
void SAMPSendMail(SAMP_HANDLE_T * pHandle, SAMP_MAIL_T * pMail);

/**
 * @brief 释放已处理邮箱函数
 * 释放已完成处理的邮箱，将其状态标记为空闲，以便重新使用
 * @note 此函数检查已用环，将对应的邮箱标记为isIdle=true
 * @param pHandle 指向邮箱句柄的指针
 * @param pLastUsedIndex 指向上次处理完成索引的指针
 * @retval 返回被释放的邮箱数量
 */
unsigned int SAMPFreeMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastUsedIndex);

/**
 * @brief 接收邮箱函数
 * 从可用环中获取待处理的邮箱，获取对端核心发送的数据
 * @note 此函数检查可用环，获取新的邮箱数据，类似VirtIO的buffer消费
 * @param pHandle 指向邮箱句柄的指针
 * @param pLastAvailIndex 指向上次接收完成索引的指针
 * @retval 成功时返回接收到的邮箱指针，无新邮箱时返回NULL
 */
SAMP_MAIL_T * SAMPReceiveMail(SAMP_HANDLE_T * pHandle, unsigned int * pLastAvailIndex);

#endif // _SIMPLEAMP_H_