/* emos PC 端验证：每圈循环模拟推进 100ms tick，输出完全确定。
 * 两个独立测试，命令行选择，公共的调度驱动循环在 main 里跑一遍：
 *   ./emos.exe event   只跑事件测试（周期 / 一次性 / 立即触发与重发）
 *   ./emos.exe msg     只跑消息测试（tx 周期发消息，rx 在 SYS_EVENT_MSG 里收空队列并释放）
 *   ./emos.exe         两个都跑 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emos.h"
#include "emos_msg.h"

static emos_tick_t sim_tick = 0;

static emos_tick_t tick_ms( void )
{
    return sim_tick;
}

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

static void test_events( void )
{
    emos_taskid_t led, work;

    printf( "==== 事件测试 ====\n" );
    if(( emos_task_register( task_led,  &led )  != EMOS_OK )||
       ( emos_task_register( task_work, &work ) != EMOS_OK ))
    {
        fprintf( stderr, "task register failed\n" );
        return;
    }

    emos_event_post_every( led,  EMOS_EV(EV_LED_BLINK),    500 );  
    emos_event_post(       led,  EMOS_EV(EV_LED_BUTTON) );       
    emos_event_post_after( work, EMOS_EV(EV_WORK_ONESHOT), 2000 ); 
}


typedef struct {
    uint16_t seq;
    char     text[24];    
} test_msg_t;

static uint16_t msg_seq = 0;
static emos_taskid_t g_rx;        

static void send_msg_to_rx( const char *tag )
{
    test_msg_t *m = (test_msg_t *)emos_msg_allocate( sizeof( test_msg_t ) );
    if ( m == NULL )
    {
        printf( "[%5u ms] tx   : allocate FAILED (%s)\n", (unsigned)tick_ms(), tag );
        return;
    }
    m->seq = msg_seq++;
    snprintf( m->text, sizeof( m->text ), "hello #%u from tx", (unsigned)m->seq );
    if ( emos_msg_send( g_rx, (uint8_t *)m ) != EMOS_OK )
    {
        printf( "[%5u ms] tx   : send FAILED\n", (unsigned)tick_ms() );
        emos_msg_deallocate( (uint8_t *)m );
        return;
    }
    printf( "[%5u ms] tx   : sent \"%s\"\n", (unsigned)tick_ms(), m->text );
}

EMOS_DEFINE_EVENTS( EV_TX, EV_TX_SEND_MSG );

static void task_tx( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    if( events & EMOS_EV(EV_TX_SEND_MSG) )
    {
        send_msg_to_rx( "periodic" );
    }
}

static void task_rx( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    if( events & EMOS_EV_SYS_MSG )
    {
        uint8_t *p;
        while( ( p = emos_msg_receive( id ) ) != NULL )
        {
            test_msg_t *m = (test_msg_t *)p;
            printf( "[%5u ms] rx   : got \"%s\" (seq=%u)\n",
                    (unsigned)tick_ms(), m->text, (unsigned)m->seq );
            emos_msg_deallocate( p );
        }
    }
}

static void test_msg( void )
{
    emos_taskid_t tx;

    printf( "==== 消息测试 ====\n" );
    if(( emos_task_register( task_tx, &tx )   != EMOS_OK )||
       ( emos_task_register( task_rx, &g_rx ) != EMOS_OK ))
    {
        fprintf( stderr, "task register failed\n" );
        return;
    }

    emos_event_post_every( tx, EMOS_EV(EV_TX_SEND_MSG), 1000 );  
    send_msg_to_rx( "boot" );                                   
}

int main( int argc, char **argv )
{
    const char *which = ( argc > 1 ) ? argv[1] : "";

    emos_init(tick_ms);

    if ( which[0] == '\0' )                   
    {
        test_events();
        test_msg();
    }
    else if ( strcmp( which, "event" ) == 0 )
    {
        test_events();
    }
    else if ( strcmp( which, "msg" ) == 0 )
    {
        test_msg();
    }
    else
    {
        fprintf( stderr, "usage: %s [event|msg]\n", argv[0] );
        return 1;
    }

    for( uint32_t loop = 0; loop < 61; loop++ )
    {
        sim_tick += 100;
        emos_run();
    }

    printf( "test done.\n" );
    return 0;
}
