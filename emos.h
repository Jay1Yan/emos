#ifndef EMOS_H
#define EMOS_H

#include <stdint.h>
#include <stddef.h>
#include "emos_config.h"

/*  类型 */
typedef uint16_t emos_event_t;     
typedef uint8_t  emos_taskid_t;     
typedef uint32_t emos_tick_t; 
typedef void (*emos_task_cb_t)(emos_taskid_t id,emos_event_t  events );

typedef emos_tick_t (*emos_tick_get_t)(void);

typedef struct {
    
    emos_event_t events;
    emos_task_cb_t task_cb;
}emos_task_t;

typedef struct {
    uint8_t vaild;
    emos_taskid_t id;
    emos_event_t events;
    emos_tick_t  expire;
    emos_tick_t  period;    
}emos_delay_events_t;

typedef enum {
    EMOS_OK = 0,
    EMOS_TASK_ARRAY_FULL, 
    EMOS_TASK_NO_EXIST, 
    EMOS_DELAY_EVENT_ARRAY_FULL, 
    EMOS_ERR_BUSY,        
} emos_task_status_t;

/* 函数 */
void emos_init( emos_tick_get_t tick_get );
emos_task_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid );
emos_task_status_t emos_task_delete( emos_taskid_t taskid );
void emos_run( void );

emos_task_status_t emos_event_post( emos_taskid_t id, emos_event_t events );
emos_task_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_task_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_task_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events );

#endif /* EMOS_H */