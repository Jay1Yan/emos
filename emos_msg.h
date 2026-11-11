/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 yyj
 */
#include "emos_config.h"
#include "emos.h"

void emos_msg_init(emos_msg_t *msg);
uint8_t emos_msg_is_pending(emos_msg_t *msg);
uint8_t* emos_msg_allocate( uint16_t len );
void emos_msg_deallocate( uint8_t *msg );
emos_status_t emos_msg_send( emos_taskid_t id, uint8_t *msg);
uint8_t* emos_msg_receive( emos_taskid_t id );