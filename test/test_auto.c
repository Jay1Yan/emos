#include <stdio.h>
#include <stdlib.h>
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

static uint16_t fail_cnt = 0;

#define TEST_ASSERT(cond)  do { \
    if ( !(cond) ) { fail_cnt++; \
        printf( "    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond ); } \
} while (0)

static void run_ticks( uint16_t runs )
{
    for ( uint16_t i = 0; i < runs; i++ )
    {
        sim_tick += 100;
        emos_run();
    }
}


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

static void test_periodic_event( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_every( t, EMOS_EV(EV_BLINK), 500 ) == EMOS_OK );

    run_ticks( 21 );

    TEST_ASSERT( g_blink_cnt == 4 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

static void test_oneshot_event( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_after( t, EMOS_EV(EV_ONESHOT), 300 ) == EMOS_OK );

    run_ticks( 20 );

    TEST_ASSERT( g_oneshot_cnt == 1 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

static void test_unarrived_cancel( void )
{
    emos_taskid_t t;

    TEST_ASSERT( emos_task_register( task_sensor, &t ) == EMOS_OK );
    TEST_ASSERT( emos_event_post_after( t, EMOS_EV(EV_ONESHOT), 1000 ) == EMOS_OK );

    run_ticks( 3 );
    TEST_ASSERT( emos_event_unarrived_cancel( t, EMOS_EV(EV_ONESHOT) ) == EMOS_OK );
    run_ticks( 27 ); 

    TEST_ASSERT( g_oneshot_cnt == 0 );
    TEST_ASSERT( emos_task_delete( t ) == EMOS_OK );
}

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
    TEST_ASSERT( emos_msg_receive( rx ) == NULL );
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );
}

static void test_msg_queue_full( void )
{
    emos_taskid_t rx;
    test_msg_t *m;

    TEST_ASSERT( emos_task_register( task_rx, &rx ) == EMOS_OK );

    for ( uint16_t i = 0; i <= EMOS_MSG_LEN_MAX; i++ ) 
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
            emos_msg_deallocate( (uint8_t *)m );   
        }
    }

    run_ticks( 1 );

    TEST_ASSERT( g_rx_cnt == EMOS_MSG_LEN_MAX );
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );
}

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

static void test_mem_exhaust( void )
{
    uint8_t *blk[EMOS_MEM_POOL_USER_BLK_CNT + 2];
    uint16_t n = count_free_blocks( 128 );
    uint16_t i;

    TEST_ASSERT( n >= 1 );

    for ( i = 0; i < n; i++ )            
    {
        blk[i] = emos_msg_allocate( 128 );
        TEST_ASSERT( blk[i] != NULL );
    }
    TEST_ASSERT( emos_msg_allocate( 128 ) == NULL ); 

    emos_msg_deallocate( blk[0] );
    TEST_ASSERT( emos_msg_allocate( 128 ) != NULL );
}


static void test_delete_drains_queue( void )
{
    emos_taskid_t rx;
    test_msg_t *m;
    uint16_t n_fresh, n_after;

    n_fresh = count_free_blocks( sizeof(test_msg_t) ); 

 
    emos_init( tick_ms );
    TEST_ASSERT( emos_task_register( task_rx, &rx ) == EMOS_OK );
    m = (test_msg_t *)emos_msg_allocate( sizeof(test_msg_t) );
    TEST_ASSERT( m != NULL );
    TEST_ASSERT( emos_msg_send( rx, (uint8_t *)m ) == EMOS_OK );
    TEST_ASSERT( emos_task_delete( rx ) == EMOS_OK );


    n_after = count_free_blocks( sizeof(test_msg_t) );
    TEST_ASSERT( n_after == n_fresh );
}


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
        emos_init( tick_ms );        

        g_events_seen = 0;          
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
