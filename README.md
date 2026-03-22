# SimpleAMP - 异构多核轻量级通信框架

SimpleAMP是一个专为异构多核系统设计的轻量级通信框架，旨在提供不同核心间的高效数据传输机制。它基于共享内存和环形缓冲区的设计理念，实现了类似VirtIO的消息传递机制。

## 项目概述

SimpleAMP通过邮箱（Mail）机制实现核心间的通信，支持：
- 异构多核系统间的数据传输
- 高效的内存共享机制
- 类似VirtIO的缓冲区管理方式
- 轻量级且易于集成

## 主要特性

- **轻量级设计**：代码简洁，资源占用少
- **高效传输**：基于共享内存的零拷贝数据传输
- **类型安全**：支持只读（RO）和可写（W）两种邮箱类型
- **线程安全**：使用内存屏障保证操作顺序
- **易于集成**：简单的API接口，方便移植

## 架构设计

### 核心数据结构

#### SAMP_MAIL_T（邮箱）
```c
typedef struct {
    SAMP_MAIL_TYPE_T type;  // 邮箱类型：只读或可写
    bool isIdle;            // 空闲状态标志
    unsigned int seq;       // 序列号
    unsigned int length;    // 数据长度
    void * address;         // 数据地址
} SAMP_MAIL_T;
```

#### SAMP_HANDLE_T（句柄）
```c
typedef struct {
    unsigned int num;          // 邮箱总数
    unsigned int availIndex;   // 可用索引
    unsigned int usedIndex;    // 已用索引
    unsigned int * pAvailRing; // 可用环数组
    unsigned int * pUsedRing;  // 已用环数组
    SAMP_MAIL_T * pMail;       // 邮箱数组
} SAMP_HANDLE_T;
```

### 双环缓冲区机制

SimpleAMP采用双环缓冲区设计：
- **可用环（Available Ring）**：记录待处理的邮箱序列号
- **已用环（Used Ring）**：记录已处理的邮箱序列号

这种设计确保了生产者和消费者之间的高效同步。

## API接口

### 初始化
```c
void SAMPInit(SAMP_HANDLE_T * pHandle, 
              unsigned int * pAvailRing, 
              unsigned int * pUsedRing, 
              SAMP_MAIL_T * pMail, 
              unsigned int num);
```

初始化SimpleAMP句柄及相关数据结构。

### 获取空闲邮箱
```c
SAMP_MAIL_T * SAMPGetIdleMail(SAMP_HANDLE_T * pHandle);
```

获取一个空闲状态的邮箱用于数据传输。

### 发送邮箱
```c
void SAMPSendMail(SAMP_HANDLE_T * pHandle, SAMP_MAIL_T * pMail);
```

将指定邮箱添加到可用环中，通知对端核心有新数据。

### 轮询接收邮箱
```c
SAMP_MAIL_T * SAMPPollMail(SAMP_HANDLE_T * pHandle, 
                           unsigned int lastAvailIndex);
```

轮询检查是否有新的邮箱消息，但不更新内部索引。

### 标记已处理邮箱
```c
bool SAMPTabUsedMail(SAMP_HANDLE_T * pHandle, 
                     unsigned int * pLastAvailIndex);
```

标记邮箱为已处理，并将其添加到已用环中以供回收。

### 释放邮箱
```c
unsigned int SAMPFreeMail(SAMP_HANDLE_T * pHandle, 
                          unsigned int * pLastUsedIndex);
```

释放已完成处理的邮箱，使其可重新使用。

## 使用示例

SimpleAMP的典型应用场景是STM32双核系统（如CM4/CM7）的通信。以下是基本使用流程：

### 1. 初始化
```c
#define MAIL_NUM 16
SAMP_HANDLE_T sampHandle;
unsigned int availRing[MAIL_NUM];
unsigned int usedRing[MAIL_NUM];
SAMP_MAIL_T mails[MAIL_NUM];
char dataBuffer[MAIL_NUM][MAX_DATA_LEN];

// 初始化SimpleAMP
SAMPInit(&sampHandle, availRing, usedRing, mails, MAIL_NUM);

// 配置邮箱属性
for(int i = 0; i < MAIL_NUM; i++) {
    mails[i].address = &dataBuffer[i];
    mails[i].length = 0;
    mails[i].type = SAMP_MAIL_TYPE_RO;
}
```

### 2. 数据发送方（如CM7）
```c
// 获取空闲邮箱
SAMP_MAIL_T * pMail = SAMPGetIdleMail(&sampHandle);
if(pMail != NULL) {
    // 填充数据
    strncpy((char*)pMail->address, data, MAX_DATA_LEN-1);
    pMail->length = strlen(data);
    
    // 发送邮箱
    SAMPSendMail(&sampHandle, pMail);
    
    // 通知接收方有新数据（通常使用硬件信号量）
    // HAL_HSEM_Release(HSEM_ID_0, 0);
}
```

### 3. 数据接收方（如CM4）- 新API使用方法
```c
// 在中断或轮询中接收数据
unsigned int lastAvailIndex = 0;  // 需要维护此变量

// 轮询检查是否有新数据
SAMP_MAIL_T * pMail = SAMPPollMail(&sampHandle, lastAvailIndex);
if(pMail != NULL) {
    // 处理接收到的数据
    process_data(pMail->address, pMail->length);
    
    // 标记邮箱为已处理，使其可以被发送方回收
    SAMPTabUsedMail(&sampHandle, &lastAvailIndex);
    
    // 通知发送方邮箱已处理完毕
    // HAL_HSEM_Release(HSEM_ID_0, 0);
}
```

## 示例应用

本项目包含完整的STM32H7双核示例：
- **CM7端**：接收串口输入并通过SimpleAMP发送给CM4
- **CM4端**：接收SimpleAMP消息并通过串口输出

## 移植指南

要将SimpleAMP移植到其他平台，需要：

1. 确保两个核心间能够共享指定的内存区域
2. 实现硬件同步机制（如硬件信号量）
3. 根据系统需求调整邮箱数量和大小
4. 确保原子操作和内存屏障的正确实现

## 注意事项

- 必须确保邮箱数据结构在两个核心间共享
- 合理设置邮箱数量以平衡性能和内存占用
- 使用适当的同步机制防止竞态条件
- 注意内存对齐要求

## 许可证

请参阅项目根目录下的LICENSE文件。
