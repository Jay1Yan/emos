#include "emos_mem.h"
#include "emos.h"

typedef struct {
    uint16_t blk_size;  
    uint16_t blk_cnt;    
} emos_mem_pool_cfg_t;

#define EMOS_MEM_MAP_WORDS ((EMOS_MEM_POOL_BLK_MAX + 31u) / 32u)

typedef struct {
    uint16_t blk_size; 
    uint16_t blk_cnt;                      
    uint16_t blk_free;                      
    uint8_t *buf;                              
    uint32_t map[EMOS_MEM_MAP_WORDS];          
} emos_mem_pool_t;

static const emos_mem_pool_cfg_t emos_mem_cfg_tab[] = {
    { .blk_size = sizeof(emos_task_t)*EMOS_TASK_MAX,.blk_cnt =1},
    { .blk_size = sizeof(emos_delay_events_t)*EMOS_DELAY_EVENTS_MAX,.blk_cnt = 1},
    { .blk_size = EMOS_MEM_POOL_USER_BLK_SIZE,.blk_cnt = EMOS_MEM_POOL_USER_BLK_CNT },
};

#define EMOS_MEM_POOL_NUM   (sizeof(emos_mem_cfg_tab) / sizeof(emos_mem_cfg_tab[0]))

static emos_mem_pool_t emos_mem_pool[EMOS_MEM_POOL_NUM];

#define EMOS_MEM_BUF_SIZE (\
    EMOS_MEM_POOL_USER_BLK_CNT*EMOS_MEM_POOL_USER_BLK_SIZE+\
    sizeof(emos_task_t)*EMOS_TASK_MAX+\
    sizeof(emos_delay_events_t)*EMOS_DELAY_EVENTS_MAX)

static uint8_t emos_mem_buf[EMOS_MEM_BUF_SIZE];

void emos_mem_init(void)
{
    uint32_t mem_buf_site = 0;
    for(uint8_t i = 0;i < EMOS_MEM_POOL_NUM;i++) 
    {
        const emos_mem_pool_cfg_t *cfg = &emos_mem_cfg_tab[i];
        EMOS_ASSERT(cfg->blk_size%sizeof(uint32_t) == 0);
        EMOS_ASSERT(cfg->blk_cnt <= EMOS_MEM_POOL_BLK_MAX);
        emos_mem_pool[i].blk_size = cfg->blk_size;
        emos_mem_pool[i].blk_cnt = cfg->blk_cnt;
        emos_mem_pool[i].blk_free = cfg->blk_cnt;
        emos_mem_pool[i].buf = emos_mem_buf+mem_buf_site;
        for ( uint8_t w = 0; w < EMOS_MEM_MAP_WORDS; w++ )
        {
            emos_mem_pool[i].map[w] = 0;    /* 重 init 可用：清掉上次的占用位 */
        }
        mem_buf_site += cfg->blk_cnt*cfg->blk_size;
    }
    EMOS_ASSERT(mem_buf_site<=EMOS_MEM_BUF_SIZE);
}

void * emos_mem_malloc( uint16_t size )
{
    EMOS_ASSERT( size != 0 );
    EMOS_ENTER_CRITICAL(); 
    for ( uint8_t i = 0; i < EMOS_MEM_POOL_NUM; i++ )
    {
        emos_mem_pool_t *p = &emos_mem_pool[i];
        if ( p->blk_size < size )
        {
            continue;   
        } 
        if ( p->blk_free == 0 )
        {
           continue;   
        }  
        uint8_t words = sizeof( p->map ) / sizeof( p->map[0] );
        for ( uint8_t w = 0; w < words; w++ )
        {
            uint16_t base = w * 32;
            if ( base >= p->blk_cnt )
            {
                break;    
            } 
            uint32_t valid = ( p->blk_cnt - base >= 32 ) ?\
            0xFFFFFFFFUL: ( ( uint32_t )1u << ( p->blk_cnt - base ) ) - 1u;
            uint32_t word  = p->map[w] | ~valid;
            if ( word == 0xFFFFFFFFUL )
            {
                continue;
            } 
            uint32_t freebits = ~word;
            uint8_t  b = 0;
            while ( ( freebits & 1u ) == 0u )
            { 
                freebits >>= 1;
                b++;
            }
            p->map[w] |= ( uint32_t )1u << b;
            p->blk_free--;
            EMOS_EXIT_CRITICAL()  
            return p->buf + ( uint32_t )( base + b ) * p->blk_size;
        }
    }
    EMOS_EXIT_CRITICAL();  
    return NULL;
}

void emos_mem_free( void *ptr )
{
    if ( ptr == NULL ) 
    {
        return;  
    }
    EMOS_ENTER_CRITICAL(); 
    for ( uint8_t i = 0; i < EMOS_MEM_POOL_NUM; i++ )         
    {
        emos_mem_pool_t *p = &emos_mem_pool[i];
        uint8_t *end = p->buf + ( uint32_t )p->blk_cnt * p->blk_size;

        if ( ( uint8_t * )ptr < p->buf || ( uint8_t * )ptr >= end )
            continue;                                       

        uint32_t off = ( uint32_t )(( uint8_t * )ptr - p->buf);
        EMOS_ASSERT( off % p->blk_size == 0 );               

        uint16_t idx = ( uint16_t )( off / p->blk_size );      

        EMOS_ASSERT( ( p->map[idx / 32u] >> ( idx % 32u ) ) & 1u ); 
        p->map[idx / 32u] &= ~( ( uint32_t )1u << ( idx % 32u ) );
        p->blk_free++;
        EMOS_EXIT_CRITICAL();  
        return;
    }
    EMOS_ASSERT( 0 );  
}
