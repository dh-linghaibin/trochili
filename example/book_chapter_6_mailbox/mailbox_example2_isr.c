#include "example.h"
#include "trochili.h"

#if (EVB_EXAMPLE == CH6_MAILBOX_EXAMPLE2)

/* 用户线程参数 */
#define THREAD_LED_STACK_BYTES  (512)
#define THREAD_LED_PRIORITY     (5)
#define THREAD_LED_SLICE        (20)

/* 用户线程定义 */
static TThread ThreadLed;

/* 用户线程栈定义 */
static TBase32 ThreadLedStack[THREAD_LED_STACK_BYTES/4];


/* 用户邮件类型定义 */
typedef struct
{
    TIndex Index;
    TByte Value;
} TLedMail;


/* 用户邮箱和邮件定义 */
static TMailbox LedMailbox;
static TLedMail LedMail;

/* Led线程的主函数 */
static void ThreadLedEntry(TArgument data)
{
    TError error;
    TState state;

    TLedMail* pMail;
    while (eTrue)
    {
        /* Led线程以阻塞方式接收邮件 */
        state = TclReceiveMail(&LedMailbox, (TMail*)(&pMail),
                               TCLO_IPC_WAIT, 0, &error);
        if (state == eSuccess)
        {
            EvbLedControl(pMail->Index, pMail->Value);
        }
    }
}

/* 评估板按键中断处理函数 */
static TBitMask EvbKeyISR(TArgument data)
{
    TState state;
    TError error;

    TLedMail* pMail = &LedMail;
    static int turn = 0;
    int keyid;

    keyid = EvbKeyScan();
    if (keyid)
    {
        if (turn % 2)
        {
            /* Key ISR以非阻塞方式发送紧急邮件 */
            LedMail.Index = LED1;
            pMail->Value =  LED_ON;
            state = TclIsrSendMail(&LedMailbox, (TMail*)(&pMail), TCLO_IPC_UARGENT, &error);
            TCLM_ASSERT((state == eSuccess), "");
            TCLM_ASSERT((error == TCLE_IPC_NONE), "");
        }
        else
        {
            /* Key ISR以非阻塞方式发送普通邮件 */
            LedMail.Index = LED1;
            pMail->Value = LED_OFF;
            state = TclIsrSendMail(&LedMailbox, (TMail*)(&pMail), (TOption)0, &error);
            TCLM_ASSERT((state == eSuccess), "");
            TCLM_ASSERT((error == TCLE_IPC_NONE), "");
        }
        turn++;

    }

    return 0;
}

/* 用户应用程序入口函数 */
static void AppSetupEntry(void)
{
    TState state;
    TError error;

    /* 设置和KEY相关的外部中断向量 */
    state = TclSetIrqVector(KEY_IRQ_ID, &EvbKeyISR, (TArgument)0, &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_IRQ_NONE), "");

    /* 初始化邮箱 */
    state = TclCreateMailbox(&LedMailbox, "mbox", TCLP_IPC_DEFAULT, &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_IPC_NONE), "");

    /* 初始化Led设备控制线程 */
    state = TclCreateThread(&ThreadLed, "thread led",
                            &ThreadLedEntry, (TArgument)(&LedMailbox),
                            ThreadLedStack, THREAD_LED_STACK_BYTES,
                            THREAD_LED_PRIORITY, THREAD_LED_SLICE, &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_THREAD_NONE), "");

    /* 激活Led设备控制线程 */
    state = TclActivateThread(&ThreadLed, &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_THREAD_NONE), "");
}


/* 处理器BOOT之后会调用main函数，必须提供 */
int main(void)
{
    /* 注册各个内核函数,启动内核 */
    TclStartKernel(&AppSetupEntry,
                   &OsCpuSetupEntry,
                   &EvbSetupEntry,
                   &EvbTraceEntry);
    return 1;
}



#endif
