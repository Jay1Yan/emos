/* emos PC 端验证：每圈循环模拟推进 100ms tick，共跑 6 秒，观察事件调度。
 * 用模拟 tick 而非真实时钟：输出完全确定，且不受 WSL/不同平台时钟精度影响。 */
#include <stdio.h>
#include <stdlib.h>
#include "emos.h"

/* 事件表：每个任务独立一张，从 bit0 编起；高4位系统保留 */
/* 模拟硬件 tick，单位 ms；由主循环每圈推进 100ms */
static emos_tick_t sim_tick = 0;

static emos_tick_t tick_ms( void )
{
    return sim_tick;
}

/* emos_config.h 中 EMOS_ASSERT 需要的失败处理 */
void emos_assert_fail( const char *file, int line )
{
    fprintf( stderr, "[ASSERT] %s:%d\n", file, line );
    abort();
}

EMOS_DEFINE_EVENTS( EV_LED, EV_LED_BLINK, EV_LED_BUTTON );

static void task_led( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    if( events & EMOS_EV(EV_LED_BLINK) )
    {
        printf( "[%5u ms] LED  : toggle\n", (unsigned)tick_ms() );
    }
    if( events & EMOS_EV(EV_LED_BUTTON) )
    {
        printf( "[%5u ms] LED  : button pressed\n", (unsigned)tick_ms() );
        emos_event_post(id, EMOS_EV(EV_LED_BUTTON) );
    }
}

EMOS_DEFINE_EVENTS( EV_WORK, EV_WORK_ONESHOT );

static void task_work( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    if( events & EMOS_EV(EV_WORK_ONESHOT) )
    {
        printf( "[%5u ms] work : one-shot fired (should print only once)\n", (unsigned)tick_ms() );
    }
}

int main( void )
{
    emos_taskid_t led, work;

    emos_init(tick_ms);

    if(( emos_task_register( task_led,  &led )  != EMOS_OK )||
       ( emos_task_register( task_work, &work ) != EMOS_OK ))
    {
        fprintf( stderr, "task register failed\n" );
        return 1;
    }

    emos_event_post_every( led,  EMOS_EV(EV_LED_BLINK),    500 );  /* 每 500ms 闪烁   */
    emos_event_post(       led,  EMOS_EV(EV_LED_BUTTON) );         /* 立即触发一次    */
    emos_event_post_after( work, EMOS_EV(EV_WORK_ONESHOT), 2000 ); /* 2s 后一次性触发 */

    for( uint32_t loop = 0; loop < 60; loop++ )   /* 每圈 100ms，共 6s */
    {
        sim_tick += 100;
        emos_run();
    }

    printf( "test done.\n" );
    return 0;
}
