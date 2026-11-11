# emos

主要用于小内存 MCU 上的事件驱动调度器，纯 C代码，不依赖任何外部库，MIT的版权声明，都可以可以改，只要你看了感觉能帮你解决一些问题。

## 实现目的

有些MCU开发，内存资源很少，使用RTOS开销太大，然后自己用状态机，状态机多了会导致代码逻辑结构
变得复杂和混乱，到处都是`if (flag_xxx)`，延时也到处都是状态标志，反正我感觉会变得很混乱。
我是在使用沁恒的芯片的时候,看到他的蓝牙协议栈实现的时候，使用了一个TMOS的调度器，本来想直接
找源码来使用，但是发现他没有开源，所以自己写了一个，而且现在非常小，一晚上或者1,2小时就能读完，然后改。

## 下面是一个使用举例

中断里——post 事件，处理事情到任务里干：

```c
#include "emos.h"

EMOS_DEFINE_EVENTS( EV_APP, EV_LED_BLINK, EV_KEY );

static emos_taskid_t g_led;

static void task_led( emos_taskid_t id, emos_event_t events )
{
    if ( events & EMOS_EV(EV_LED_BLINK) ) { led_toggle(); }
    if ( events & EMOS_EV(EV_KEY) )       { led_toggle(); }
}

int main( void )
{
    emos_init( tick_ms );                 
    emos_task_register( task_led, &g_led );
    emos_event_post_every( g_led, EMOS_EV(EV_LED_BLINK), 500 );

    for ( ;; ) { emos_run(); }
}

void EXTI_Handler( void )
{
    emos_event_post( g_led, EMOS_EV(EV_KEY) );
}
```

每个任务一个 16 位事件字，哪路事件来了查位就知道，不用再发明全局标志。
定时的事用 post_after / post_every，没有死等。模块之间传数据走消息队列：
发送方申请内存、投递，收不进去自己回收；接收方在自己任务里收空、释放。

## 内存

默认配置大约 3.2KB RAM：任务 10 个（每个 56B）、延时事件 10 个、用户内存池 20×128，用户内存你可以自己修改，支持的是多级的内存池，你可以添加不同大小的内存池和块，都在 emos_config.h 里改，砍到 1KB 以下也能跑。没有堆，所有内存在链接时就定死，运行时不会冒出意外的分配。前面是系统预留，所以的内存都是从这个内存池里面分配出来的。
```c
static const emos_mem_pool_cfg_t emos_mem_cfg_tab[] = {
    { .blk_size = sizeof(emos_task_t)*EMOS_TASK_MAX,.blk_cnt =1},
    { .blk_size = sizeof(emos_delay_events_t)*EMOS_DELAY_EVENTS_MAX,.blk_cnt = 1},
    { .blk_size = EMOS_MEM_POOL_USER_BLK_SIZE,.blk_cnt = EMOS_MEM_POOL_USER_BLK_CNT },
};
```

## API

下面这个接口可以在中断里调用（内部有临界区保护）：

```c
emos_status_t emos_event_post( emos_taskid_t id, emos_event_t events );
emos_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events );
emos_status_t emos_event_unarrived_cancel( emos_taskid_t id, emos_event_t events );
emos_status_t emos_msg_send( emos_taskid_t id, uint8_t *msg );
void        *emos_mem_malloc( uint16_t size );
void         emos_mem_free( void *ptr );
```

这些只在主循环里调用：

```c
void         emos_init( emos_tick_get_t tick_get );
emos_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid );
emos_status_t emos_task_delete( emos_taskid_t taskid );
uint8_t      *emos_msg_receive( emos_taskid_t id );
void         emos_run( void );
```

## 移植

配置修改emos_config.h就可以了：

时基——给 `emos_init` 一个返回毫秒计数的函数，SysTick、RTC 都行，`uint32_t` 可回绕，不需要
使用定时器中断，能读值就可以了。

临界区宏——成对、同一作用域、save/restore 式，支持嵌套：

```c
/* Cortex-M（STM32 等） */
#define EMOS_ENTER_CRITICAL()  uint32_t primask_save = __get_PRIMASK(); __disable_irq()
#define EMOS_EXIT_CRITICAL()   __set_PRIMASK( primask_save )

/* 沁恒 RISC-V（CH32V/CH58x*/
#define EMOS_ENTER_CRITICAL()  uint32_t mstatus_save = __get_MSTATUS(); __disable_irq()
#define EMOS_EXIT_CRITICAL()   __set_MSTATUS( mstatus_save )
```

## 测试

```
cd test
make test
```
你可以在电脑上编译测试，上面的测试就是这么跑的。
9 个用例，用假时钟驱动，输出完全确定，全过退出码 0。
改了修改内核代码之后，你可以跑一遍，几秒钟就知道有没有改坏东西。

## 还没做

- 低功耗：idle 钩子、按最近到期事件决定睡眠时长，电池供电的应用需要
- 统计：任务最长执行耗时、内存池高水位，用来调配置
- 真机验证：目前只在电脑上测过

## License

MIT
