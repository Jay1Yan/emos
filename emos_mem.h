#include <stdint.h>
#include "emos_config.h"

void emos_mem_init(void);
void *emos_mem_malloc( uint16_t size );  
void emos_mem_free( void *ptr );