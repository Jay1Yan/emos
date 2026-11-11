/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 yyj
 */
#ifndef EMOS_H
#define EMOS_H

#include <stdint.h>
#include <stddef.h>
#include "emos_config.h"
 
#define EMOS_SYS_EVENT_RESERVE       4
#define EMOS_SYS_EVENT_BASE         (8 * (int)sizeof( emos_event_t ) - EMOS_SYS_EVENT_RESERVE )

#define EMOS_DEFINE_EVENTS( prefix, ... )                          \
    enum { __VA_ARGS__, prefix##_N };                              \
    typedef char prefix##_events_fit                               \
        [( prefix##_N <= EMOS_SYS_EVENT_BASE ) ? 1 : -1]

#define EMOS_EV( bit )  ((emos_event_t)1u << (bit))        

#define EMOS_EV_SYS_MSG         EMOS_EV( EMOS_SYS_EVENT_BASE)  

typedef uint16_t emos_event_t;     
typedef uint8_t  emos_taskid_t;     
typedef uint32_t emos_tick_t; 
typedef void (*emos_task_cb_t)(emos_taskid_t id,emos_event_t  events );

typedef emos_tick_t (*emos_tick_get_t)(void);

typedef struct {
    void * queue[EMOS_MSG_LEN_MAX];
    uint16_t total;
    uint16_t head;
    uint16_t tail;
}emos_msg_t;

typedef struct {
    
    emos_event_t events;
    emos_task_cb_t task_cb;
    emos_msg_t msg;
}emos_task_t;

typedef struct {
    uint8_t valid;
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
    EMOS_MSG_QUEUE_FULL,       
} emos_status_t;

void emos_init( emos_tick_get_t tick_get );
emos_status_t emos_task_register( emos_task_cb_t task_cb, emos_taskid_t *taskid );
emos_status_t emos_task_delete( emos_taskid_t taskid );
void emos_run( void );

emos_status_t emos_event_post( emos_taskid_t id, emos_event_t events );
emos_status_t emos_event_post_after( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_status_t emos_event_post_every( emos_taskid_t id, emos_event_t events, emos_tick_t tick );
emos_status_t emos_event_cancel( emos_taskid_t id, emos_event_t events );
emos_status_t emos_event_unarrived_cancel( emos_taskid_t id, emos_event_t events );

#endif