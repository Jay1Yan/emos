/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 yyj
 */
#include "emos_msg.h"
#include "emos_mem.h"

extern emos_task_t *emos_task_array;

uint8_t* emos_msg_allocate( uint16_t size )
{
    return (uint8_t *)emos_mem_malloc(size);
}

void emos_msg_deallocate( uint8_t *msg )
{
    return emos_mem_free(msg);
}

void emos_msg_init(emos_msg_t *msg)
{
    msg->total = EMOS_MSG_LEN_MAX;
    msg->head = 0;
    msg->tail = 0;
}

uint8_t emos_msg_is_pending(emos_msg_t *msg)
{
    return msg->head != msg->tail?1:0;
}

emos_status_t emos_msg_send( emos_taskid_t id, uint8_t *msg)
{
    emos_status_t status;
    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ASSERT( msg != NULL );

    EMOS_ENTER_CRITICAL(); 
    if( emos_task_array[id].task_cb == NULL )
    {
        status = EMOS_TASK_NO_EXIST;
    }
    else
    {
        emos_msg_t *q = &(emos_task_array[id].msg);
        if((q->head - q->tail) < q->total)
        {
            q->queue[q->head%q->total] = (void *)msg;
            q->head++;
            status = EMOS_OK;
        }
        else
        {
            status = EMOS_MSG_QUEUE_FULL;
        }
    }
    EMOS_EXIT_CRITICAL();
    return status;  
}  

uint8_t* emos_msg_receive( emos_taskid_t id )
{
    uint8_t *msg;

    EMOS_ASSERT( id < EMOS_TASK_MAX );
    EMOS_ENTER_CRITICAL(); 
    if( emos_task_array[id].task_cb == NULL )
    {
        msg=  NULL;
    }
    else
    {
        emos_msg_t *q = &(emos_task_array[id].msg);
        if(q->head != q->tail)
        {
            msg = q->queue[q->tail%q->total];
            q->tail++;
        }
        else
        {
            msg=  NULL;
        };
    }
    EMOS_EXIT_CRITICAL();

    return msg;
}