#include "emos.h"

static emos_tick_get_t tick_get;
static emos_task_t emos_task_array[EMOS_TASK_MAX];
static emos_delay_events_t  emos_delay_events[EMOS_DELAY_EVENTS_MAX];

void emos_init( emos_tick_get_t tick_get_fn )
{
    tick_get = tick_get_fn;
}

emos_task_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid )
{
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        if(emos_task_array[id].task_cb == NULL )
        {
            emos_task_array[id].task_cb = task_cb;
            emos_task_array[id].events = 0;  
            *taskid = id;
            return EMOS_OK;
        }
    }
    return EMOS_TASK_ARRAY_FULL;    
}

emos_task_status_t emos_task_delete(emos_taskid_t taskid )
{
    EMOS_ASSERT( taskid < EMOS_TASK_MAX );
    if( emos_task_array[taskid].task_cb == NULL )
    {
        return EMOS_TASK_NO_EXIST;
    }
    emos_task_array[taskid].task_cb = NULL;
    return EMOS_OK;    
}

emos_task_status_t emos_event_post( emos_taskid_t id, emos_event_t events )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    emos_task_array[id].events |= events;
    return EMOS_OK;
}

emos_task_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    emos_task_array[id].events &= (~events);
    return EMOS_OK;
}

static emos_task_status_t emos_delay_event_insert( emos_taskid_t id,emos_event_t events, uint32_t tick,uint8_t is_every)
{
    for( uint16_t i = 0;i < EMOS_DELAY_EVENTS_MAX;i++)
    {
        if( emos_delay_events[i].vaild == 1 )
        {
            if((emos_delay_events[i].id == id)&&
                (emos_delay_events[i].events == events))
            {
                return EMOS_OK;
            }
        }
        else
        {
            emos_delay_events[i].vaild = 1;  
            emos_delay_events[i].id = id;
            emos_delay_events[i].expire = tick;
            emos_delay_events[i].events = events;
            if(is_every)
            {
                emos_delay_events[i].period = tick;   
            }
            else
            {
                emos_delay_events[i].period = 0;   
            }
            return EMOS_OK;
        }
    }
    return EMOS_DELAY_EVENT_ARRAY_FULL;
}

emos_task_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_delay_event_insert(id,events,tick,0);
}

emos_task_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_delay_event_insert(id,events,tick,1);
}

static emos_tick_t emos_tick_diff(emos_tick_t tick_cur, emos_tick_t tick_last)
{
    return tick_cur-tick_last;
}

void emos_run( void )
{
    static emos_tick_t last;
    static uint8_t inited = 0;

    emos_tick_t now = tick_get();
    emos_tick_t diff;

    if( !inited )
    {
        inited = 1;
        last = now;
    }
    diff = emos_tick_diff(now,last);
    last = now;

    for(uint16_t i =0; i <EMOS_DELAY_EVENTS_MAX;i++ )
    {
        if( emos_delay_events[i].vaild)
        {
            if( emos_delay_events[i].expire > diff)
            {
               emos_delay_events[i].expire -= diff;
            }
            else
            {
                if( emos_delay_events[i].period)
                {
                    emos_delay_events[i].expire = emos_delay_events[i].period;
                }
                else
                {
                    emos_delay_events[i].vaild = 0;
                    emos_delay_events[i].expire = 0;   
                }
                emos_event_post(emos_delay_events[i].id,emos_delay_events[i].events);
            }
        }
    }
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        emos_event_t events = emos_task_array[id].events;
        if((emos_task_array[id].task_cb != NULL)&&( events != 0))
        {
            emos_task_array[id].events = 0;  
            emos_task_array[id].task_cb(id,events);
        }
    }
}