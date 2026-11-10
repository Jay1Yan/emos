#include "emos.h"
#include "emos_mem.h"
#include "emos_msg.h"

static emos_tick_get_t tick_get;
emos_task_t *emos_task_array;
static emos_delay_events_t *emos_delay_events;

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
}

emos_task_t * emos_get_task_array(void)
{
    return emos_task_array;
}

emos_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid )
{
    EMOS_ASSERT( task_cb != NULL );
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        if(emos_task_array[id].task_cb == NULL )
        {
            emos_task_array[id].task_cb = task_cb;
            emos_task_array[id].events = 0;
            emos_msg_init(&(emos_task_array[id].msg)); 
            *taskid = id;
            return EMOS_OK;
        }
    }
    return EMOS_TASK_ARRAY_FULL;    
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
    EMOS_ASSERT( taskid < EMOS_TASK_MAX );
    if( emos_task_array[taskid].task_cb == NULL )
    {
        return EMOS_TASK_NO_EXIST;
    }
    emos_task_array[taskid].task_cb = NULL;
    emos_delay_event_delete(taskid);
    return EMOS_OK;    
}

emos_status_t emos_event_post( emos_taskid_t id, emos_event_t events )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    emos_task_array[id].events |= events;
    return EMOS_OK;
}

emos_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;
    }
    emos_task_array[id].events &= ~events;      
    return EMOS_OK;
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
    emos_delay_events[free_idx].valid  = 1;
    emos_delay_events[free_idx].id     = id;
    emos_delay_events[free_idx].expire = tick;
    emos_delay_events[free_idx].events = events;
    emos_delay_events[free_idx].period = is_every ? tick : 0;
    return EMOS_OK;
}

emos_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_delay_event_insert(id,events,tick,0);
}

emos_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, uint32_t tick )
{
    EMOS_ASSERT( tick != 0 );
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_delay_event_insert(id,events,tick,1);
}

emos_status_t emos_event_unarrived_cancel( emos_taskid_t id, emos_event_t events )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;
    }     
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
    return EMOS_OK;
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
        if( emos_delay_events[i].valid)
        {
            if( emos_delay_events[i].expire > diff)
            {
               emos_delay_events[i].expire -= diff;
            }
            else
            {
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
                emos_event_post(emos_delay_events[i].id,emos_delay_events[i].events);
            }
        }
    }
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        if( emos_task_array[id].task_cb != NULL)
        {
            if( emos_msg_is_pending(&(emos_task_array[id].msg)))
            {
                emos_task_array[id].events |=EMOS_EV_SYS_MSG;
            }
            emos_event_t events = emos_task_array[id].events;
            if( events != 0)
            {
                emos_task_array[id].events &= ~events;
                emos_task_array[id].task_cb(id,events);
            }
        }
    }
}