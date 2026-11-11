/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 yyj
 */
#include "emos.h"
#include "emos_mem.h"
#include "emos_msg.h"
#include <string.h>

static emos_tick_get_t tick_get;
emos_task_t *emos_task_array;
static emos_delay_events_t *emos_delay_events;
static emos_tick_t run_last;       
static uint8_t      run_inited = 0;

void emos_init( emos_tick_get_t tick_get_fn )
{
    tick_get = tick_get_fn;
    emos_mem_init();
    emos_task_array = (emos_task_t *)emos_mem_malloc(\
        sizeof(emos_task_t)*EMOS_TASK_MAX);
    EMOS_ASSERT( emos_task_array != NULL );
    emos_delay_events = (emos_delay_events_t *)emos_mem_malloc(\
        sizeof(emos_delay_events_t)*EMOS_DELAY_EVENTS_MAX);
    EMOS_ASSERT( emos_delay_events != NULL );
    memset( emos_task_array, 0, sizeof(emos_task_t)*EMOS_TASK_MAX );
    memset( emos_delay_events, 0, sizeof(emos_delay_events_t)*EMOS_DELAY_EVENTS_MAX );
    run_inited = 0;
}

emos_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid )
{
    emos_status_t status;

    EMOS_ASSERT( task_cb != NULL );
    status = EMOS_TASK_ARRAY_FULL;
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        EMOS_ENTER_CRITICAL(); 
        if(emos_task_array[id].task_cb == NULL )
        {
            emos_task_array[id].events = 0;
            emos_msg_init(&(emos_task_array[id].msg)); 
            emos_task_array[id].task_cb = task_cb;
            *taskid = id;
            status = EMOS_OK;
            EMOS_EXIT_CRITICAL();
            break;
        }
        EMOS_EXIT_CRITICAL();
    }
    return status;    
}

static void emos_msg_delete( emos_taskid_t id)
{
    uint8_t * p;
    while( ( p = emos_msg_receive( id ) ) != NULL )
    {
        emos_msg_deallocate( p );
    }
}

static emos_status_t emos_delay_event_delete( emos_taskid_t id)
{
    for( uint16_t i = 0;i < EMOS_DELAY_EVENTS_MAX;i++)
    {
        if( emos_delay_events[i].valid == 1 )
        {
            if(emos_delay_events[i].id == id)
            {
                emos_delay_events[i].valid = 0;
                emos_delay_events[i].id = 0;
                emos_delay_events[i].events = 0;
                emos_delay_events[i].expire = 0;
                emos_delay_events[i].period = 0;    
            }
        }
    }
    return EMOS_OK;
}

emos_status_t emos_task_delete(emos_taskid_t taskid )
{
    emos_status_t status;

    EMOS_ASSERT( taskid < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL();
    if( emos_task_array[taskid].task_cb == NULL )
    {
        status = EMOS_TASK_NO_EXIST;
    }
    else
    {
        emos_delay_event_delete(taskid);
        emos_msg_delete(taskid); 
        emos_task_array[taskid].task_cb = NULL;  
        status = EMOS_OK;   
    }
    EMOS_EXIT_CRITICAL();
    return status;    
}

emos_status_t emos_event_post( emos_taskid_t id, emos_event_t events )
{
    emos_status_t status;
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL();
    if( emos_task_array[id].task_cb == NULL)
    {
        status = EMOS_TASK_NO_EXIST;  
    }
    else
    {
        emos_task_array[id].events |= events; 
        status = EMOS_OK;   
    }
    EMOS_EXIT_CRITICAL();

    return status;
}

emos_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events )
{
    emos_status_t status;

    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL();
    if( emos_task_array[id].task_cb == NULL)
    {
        status = EMOS_TASK_NO_EXIST;
    }
    else
    {
        emos_task_array[id].events &= ~events;
        status = EMOS_OK;      
    }
    EMOS_EXIT_CRITICAL();

    return status;
}

static emos_status_t emos_delay_event_insert( emos_taskid_t id,emos_event_t events, uint32_t tick,uint8_t is_every)
{
    int16_t free_idx = -1;
    for( uint16_t i = 0;i < EMOS_DELAY_EVENTS_MAX;i++)
    {
        if( emos_delay_events[i].valid == 1 )
        {
            if((emos_delay_events[i].id == id)&&
                (emos_delay_events[i].events == events))
            {
                emos_delay_events[i].expire = tick;
                emos_delay_events[i].period = is_every ? tick : 0;
                return EMOS_OK;
            }
        }
        else if( free_idx < 0 )
        {
            free_idx = i; 
        }
    }
    if( free_idx < 0 )
    {
        return EMOS_DELAY_EVENT_ARRAY_FULL;
    }
    emos_delay_events[free_idx].id     = id;
    emos_delay_events[free_idx].expire = tick;
    emos_delay_events[free_idx].events = events;
    emos_delay_events[free_idx].period = is_every ? tick : 0;
    emos_delay_events[free_idx].valid  = 1;

    return EMOS_OK;
}

emos_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    emos_status_t status;
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL();
    if( emos_task_array[id].task_cb == NULL)
    {
        status =  EMOS_TASK_NO_EXIST;  
    }
    else
    {
        status =  emos_delay_event_insert(id,events,tick,0);    
    }
    EMOS_EXIT_CRITICAL();

    return status;
}

emos_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    emos_status_t status;
    EMOS_ASSERT( tick != 0 );
    EMOS_ASSERT( id < EMOS_TASK_MAX );

    EMOS_ENTER_CRITICAL();
    if( emos_task_array[id].task_cb == NULL)
    {
        status =  EMOS_TASK_NO_EXIST;  
    }
    else
    {
        status =  emos_delay_event_insert(id,events,tick,1);    
    }
    EMOS_EXIT_CRITICAL();

    return status;
}

emos_status_t emos_event_unarrived_cancel( emos_taskid_t id, emos_event_t events )
{
    emos_status_t status;

    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL();
    if( emos_task_array[id].task_cb == NULL)
    {
        status =  EMOS_TASK_NO_EXIST;
    }
    else
    {
        for( uint16_t i = 0;i < EMOS_DELAY_EVENTS_MAX;i++)
        {
            if(( emos_delay_events[i].valid == 1 )&&
            ( emos_delay_events[i].id == id )&&
            ( emos_delay_events[i].events == events ))
            {
                emos_delay_events[i].valid  = 0;  
                emos_delay_events[i].expire = 0;
                emos_delay_events[i].period = 0;
            }
        }
        status =  EMOS_OK;
    }     
    EMOS_EXIT_CRITICAL();

    return status;
}


static emos_tick_t emos_tick_diff(emos_tick_t tick_cur, emos_tick_t tick_last)
{
    return tick_cur-tick_last;
}

void emos_run( void )
{
    emos_tick_t now = tick_get();
    emos_tick_t diff;

    if( !run_inited )
    {
        run_inited = 1;
        run_last = now;
    }
    diff = emos_tick_diff(now,run_last);
    run_last = now;

    for(uint16_t i =0; i <EMOS_DELAY_EVENTS_MAX;i++ )
    {
        emos_event_t  fire_ev = 0;
        emos_taskid_t fire_id = 0;
        uint8_t       fire = 0;

        EMOS_ENTER_CRITICAL(); 
        if( emos_delay_events[i].valid)
        {
            if( emos_delay_events[i].expire > diff)
            {
               emos_delay_events[i].expire -= diff;
            }
            else
            {
                fire = 1;                                    
                fire_id = emos_delay_events[i].id;
                fire_ev = emos_delay_events[i].events;
                if( emos_delay_events[i].period)
                {
                    emos_tick_t next = emos_delay_events[i].expire + emos_delay_events[i].period;
                    if( next > diff )
                    {
                        emos_delay_events[i].expire = next - diff;  
                    }
                    else
                    {
                        emos_delay_events[i].expire = 0;    
                    }
                }
                else
                {
                    emos_delay_events[i].valid = 0;
                    emos_delay_events[i].expire = 0;   
                }
            }
        }
        EMOS_EXIT_CRITICAL(); 
        if ( fire )                                      
        {
            emos_event_post( fire_id, fire_ev );
        }
    }
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        if( emos_task_array[id].task_cb != NULL)
        {
            EMOS_ENTER_CRITICAL(); 
            if( emos_msg_is_pending(&(emos_task_array[id].msg)))
            {
                emos_task_array[id].events |=EMOS_EV_SYS_MSG;
            }
            emos_event_t events = emos_task_array[id].events;
            if( events != 0)
            {
                emos_task_array[id].events &= ~events;
                EMOS_EXIT_CRITICAL(); 
                emos_task_array[id].task_cb(id,events);
            }
            else
            {
                EMOS_EXIT_CRITICAL();
            }
        }
    }
}
