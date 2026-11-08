#include "emos.h"

#define EMOS_TASK_MAX               10
#define EMOS_DELAY_EVENTS_MAX       10

static emos_task_t emos_task_array[EMOS_TASK_MAX];
static emos_delay_events_t  emos_delay_events[EMOS_DELAY_EVENTS_MAX];

emos_task_status_t emos_task_register( emos_task_cb_t task_cb,emos_taskid_t *taskid )
{
    EMOS_ASSERT( emos_task_array[id].task_cb != NULL );
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

static emos_task_status_t emos_event_post( emos_taskid_t id,emos_event_t events, uint32_t ms,uint8_t is_every)
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
            emos_delay_events[i].expire = ms;
            if(is_every)
            {
                emos_delay_events[i].period = ms;   
            }
            return EMOS_OK;
        }
    }
    return EMOS_DELAY_EVENT_ARRAY_FULL;
}

emos_task_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, uint32_t ms )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_event_post(id,events,ms,0);
}

emos_task_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, uint32_t ms )
{
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    if( emos_task_array[id].task_cb == NULL)
    {
        return EMOS_TASK_NO_EXIST;  
    }
    return emos_event_post(id,events,ms,1);
}


void emos_run( void )
{
    for(emos_taskid_t id = 0;id < EMOS_TASK_MAX;id++)
    {
        if(emos_task_array[id].events != 0)
        {
            emos_task_array[id].task_cb(emos_task_array[id].events);  
        }
    }
}