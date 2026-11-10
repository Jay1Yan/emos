/* emos 自动化测试：断言代替人眼。
 * 每个 test 函数跑完由 main 汇总 PASS/FAIL；退出码 0 = 全部通过。
 *   cd test && make test
 * 每个测试开始前 sim_tick 归零、emos_init 重建内核，测试之间互不依赖；
 * 测试内部只断言自己注册的任务。
 */
#include <stdio.h>
#include <stdlib.h>
#include "emos.h"
#include "emos_msg.h"

/* ---- 假时钟：时间由测试推着走，输出完全确定 ---- */
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

/* ---- 测试框架：断言记失败不中断，跑完看总数 ---- */
static uint16_t fail_cnt = 0;

#define TEST_ASSERT(cond)  do { \
    if ( !(cond) ) { fail_cnt++; \
        printf( "    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond ); } \
} while (0)

/* 驱动：runs 次「时间 +100ms，调度一圈」 */
static void run_ticks( uint16_t runs )
{
    for ( uint16_t i = 0; i < runs; i++ )
    {
        sim_tick += 100;
        emos_run();
    }
}

/* ---- 传感器任务：只记录，不打印 ---- */
EMOS_DEFINE_EVENTS( EV_T, EV_BLINK, EV_ONESHOT, EV_BUTTON );

static emos_event_t g_events_seen;
static uint16_t     g_blink_cnt;
static uint16_t     g_oneshot_cnt;

static void task_sensor( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    g_events_seen |= events;
    if ( events & EMOS_EV(EV_BLINK) )   { g_blink_cnt++; }
    if ( events & EMOS_EV(EV_ONESHOT) ) { g_oneshot_cnt++; }
}

typedef struct { uint16_t seq; } test_msg_t;

static uint16_t g_rx_seq[16];
static uint16_t g_rx_cnt;

static void task_rx( emos_taskid_t id, emos_event_t events )
{
    (void)id;
    if ( events & EMOS_EV_SYS_MSG )
    {
        uint8_t *p;
        while ( ( p = emos_msg_receive( id ) ) != NULL )
        {
            if ( g_rx_cnt < 16 ) { g_rx_seq[g_rx_cnt] = ( (test_msg_t *)p )->seq; }
            g_rx_cnt++;
            emos_msg_deallocate( p );
        }
    }
}

/* ---- 测试用例 ---- */

/* 1. 立即事件：同一位 post 两次只送达一次（位图合并），不同位同轮都到 */
static void test_post_immediate( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post( t, EMOS_EV(EV_BLINK) ) == EMOS_OK );
    TEST_ASSERT( emos_event_post( t, EMOS_EV(EV_BLINK) ) == EMOS_OK );
    TEST_ASSERT( emos_event_post( t, EMOS_EV(EV_BUTTON) ) == EMOS_OK );

    run_ticks( 1 );

    TEST_ASSERT( g_blink_cnt == 1 );
    TEST_ASSERT( ( g_events_seen & EMOS_EV(EV_BLINK) )   != 0 );
    TEST_ASSERT( ( g_events_seen & EMOS_EV(EV_BUTTON) )  != 0 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

/* 2. 周期事件：500ms 周期。首次 run diff=0，触发点 600/1100/1600/2100 */
static void test_periodic_event( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_every( t, EMOS_EV(EV_BLINK), 500 ) == EMOS_OK );

    run_ticks( 21 );    /* 推进到 2100ms */

    TEST_ASSERT( g_blink_cnt == 4 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

/* 3. 一次性事件：300ms 后触发，且只触发一次 */
static void test_oneshot_event( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_after( t, EMOS_EV(EV_ONESHOT), 300 ) == EMOS_OK );

    run_ticks( 20 );    /* 推进到 2000ms，300ms 的事件早已到期 */

    TEST_ASSERT( g_oneshot_cnt == 1 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

/* 4. 取消未到期：挂 1000ms 的事件，300ms 时取消，到点不再触发 */
static void test_unarrived_cancel( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_after( t, EMOS_EV(EV_ONESHOT), 1000 ) == EMOS_OK );

    run_ticks( 3 );     /* 300ms */
    TEST_ASSERT( emos_event_unarrived_cancel( t, EMOS_EV(EV_ONESHOT) ) == EMOS_OK );
    run_ticks( 27 );    /* 推进到 3000ms，远超 1000ms */

    TEST_ASSERT( g_oneshot_cnt == 0 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

/* 5. 死任务与表满 */
static void test_task_life( void )
{
    emos_taskid_t id[EMOS_TASK_MAX + 1];
    emos_taskid_t t;

    TEST_ASSERT( emos_event_post( 5, EMOS_EV(EV_BLINK) ) == EMOS_TASK_NO_EXIST );

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post( t, EMOS_EV(EV_BLINK) ) == EMOS_TASK_NO_EXIST );

    for ( uint8_t i = 0; i < EMOS_TASK_MAX; i++ )
    {
        TEST_ASSERT( emos_task_register( task_sensor, &id[i] ) == EMOS_OK );
    }
    TEST_ASSERT( emos_task_register( task_sensor, &id[EMOS_TASK_MAX] )
                 == EMOS_TASK_ARRAY_FULL );
}

/* 6. 消息 FIFO 与收空 */
static void test_msg_fifo( void )
{
    emos_taskid_t rx;
    test_msg_t *m;

    TEST_ASSERT( emos_task_register( task_rx, &rx ) == EMOS_OK );

    for ( uint16_t i = 0; i < 3; i++ )
    {
        m = (test_msg_t *)emos_msg_allocate( sizeof(test_msg_t) );
        TEST_ASSERT( m != NULL );
        m->seq = i;
        TEST_ASSERT( emos_msg_send( rx, (uint8_t *)m ) == EMOS_OK );
    }

    run_ticks( 1 );

    TEST_ASSERT( g_rx_cnt == 3 );
    TEST_ASSERT( g_rx_seq[0] == 0 && g_rx_seq[1] == 1 && g_rx_seq[2] == 2 );
    TEST_ASSERT( emos_msg_receive( rx ) == NULL );    /* 已收空 */
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );
}

/* 7. 队列满：第 11 条被拒，且发送方能回收 */
static void test_msg_queue_full( void )
{
    emos_taskid_t rx;
    test_msg_t *m;

    TEST_ASSERT( emos_task_register( task_rx, &rx ) == EMOS_OK );

    for ( uint16_t i = 0; i <= EMOS_MSG_LEN_MAX; i++ )    /* 发 11 条 */
    {
        m = (test_msg_t *)emos_msg_allocate( sizeof(test_msg_t) );
        TEST_ASSERT( m != NULL );
        m->seq = i;
        if ( i < EMOS_MSG_LEN_MAX )
        {
            TEST_ASSERT( emos_msg_send( rx, (uint8_t *)m ) == EMOS_OK );
        }
        else
        {
            TEST_ASSERT( emos_msg_send( rx, (uint8_t *)m )
                         == EMOS_MSG_QUEUE_FULL );
            emos_msg_deallocate( (uint8_t *)m );    /* 发送失败，回收 */
        }
    }

    run_ticks( 1 );

    TEST_ASSERT( g_rx_cnt == EMOS_MSG_LEN_MAX );
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );
}

/* 数当前还能分到几块 size 的块：先全部占住（上限跟着配置走，防饱和），
 * 数完一起归还——状态不变，计数精确 */
static uint16_t count_free_blocks( uint16_t size )
{
    uint8_t *blk[EMOS_MEM_POOL_USER_BLK_CNT + 2];
    uint16_t n = 0;

    while ( n < EMOS_MEM_POOL_USER_BLK_CNT + 2 &&
            ( blk[n] = emos_msg_allocate( size ) ) != NULL )
    {
        n++;
    }
    for ( uint16_t i = 0; i < n; i++ )
    {
        emos_msg_deallocate( blk[i] );
    }
    return n;
}

/* 8. 内存池：分配到 NULL，释放一块又能分到 */
static void test_mem_exhaust( void )
{
    uint8_t *blk[EMOS_MEM_POOL_USER_BLK_CNT + 2];
    uint16_t n = count_free_blocks( 128 );
    uint16_t i;

    TEST_ASSERT( n >= 1 );

    for ( i = 0; i < n; i++ )                     /* 恰好分完 n 块 */
    {
        blk[i] = emos_msg_allocate( 128 );
        TEST_ASSERT( blk[i] != NULL );
    }
    TEST_ASSERT( emos_msg_allocate( 128 ) == NULL );  /* 再要没有 */

    emos_msg_deallocate( blk[0] );
    TEST_ASSERT( emos_msg_allocate( 128 ) != NULL );
    /* 不逐一归还：下一个测试 emos_init 会整体重置内存池 */
}

/* 9. 删除任务要回收队列里未取走的消息：发一条、不 run、直接删，块不能漏 */
static void test_delete_drains_queue( void )
{
    emos_taskid_t rx;
    test_msg_t *m;
    uint16_t n_fresh, n_after;

    n_fresh = count_free_blocks( sizeof(test_msg_t) );  /* 基线：与消息同尺寸 */

    /* 重建内核：发一条消息进队列，不调度，直接删任务 */
    emos_init( tick_ms );
    TEST_ASSERT( emos_task_register( task_rx, &rx ) == EMOS_OK );
    m = (test_msg_t *)emos_msg_allocate( sizeof(test_msg_t) );
    TEST_ASSERT( m != NULL );
    TEST_ASSERT( emos_msg_send( rx, (uint8_t *)m ) == EMOS_OK );
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );

    /* 若 delete 漏清队列，那条消息占的块回不来，这里就少一块 */
    n_after = count_free_blocks( sizeof(test_msg_t) );
    TEST_ASSERT( n_after == n_fresh );
}

/* ---- 测试表与总入口 ---- */
typedef void (*test_fn_t)( void );
typedef struct { const char *name; test_fn_t fn; } test_case_t;

static const test_case_t tests[] = {
    { "post_immediate",   test_post_immediate   },
    { "periodic_event",   test_periodic_event   },
    { "oneshot_event",    test_oneshot_event    },
    { "unarrived_cancel", test_unarrived_cancel },
    { "task_life",        test_task_life        },
    { "msg_fifo",         test_msg_fifo         },
    { "msg_queue_full",   test_msg_queue_full   },
    { "mem_exhaust",      test_mem_exhaust      },
    { "delete_drains",    test_delete_drains_queue },
};

#define TESTS_NUM ( sizeof(tests) / sizeof(tests[0]) )

int main( void )
{
    uint16_t tests_failed = 0;

    printf( "==== emos 自动化测试 ====\n" );
    for ( unsigned i = 0; i < TESTS_NUM; i++ )
    {
        uint16_t before = fail_cnt;

        sim_tick = 0;
        emos_init( tick_ms );        /* 每个测试全新内核 */

        g_events_seen = 0;           /* 传感器归零 */
        g_blink_cnt   = 0;
        g_oneshot_cnt = 0;
        g_rx_cnt      = 0;

        tests[i].fn();
        if ( fail_cnt != before ) { tests_failed++; }
        printf( "%-16s %s\n", tests[i].name,
                fail_cnt == before ? "PASS" : "FAIL" );
    }

    printf( "==== %u/%u 通过 ====\n",
            (unsigned)( TESTS_NUM - tests_failed ), (unsigned)TESTS_NUM );
    return tests_failed ? 1 : 0;
}
