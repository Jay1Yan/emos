/*  类型 */
typedef uint16_t emos_event_t;     
typedef uint8_t  emos_taskid_t;     
typedef uint32_t emos_tick_t; 
typedef void (*emos_task_cb_t)(emos_taskid_t id,emos_event_t  events );

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

/* 断言*/
#if defined(EMOS_DEBUG)
  #define EMOS_ASSERT(cond)  do { if (!(cond)) emos_assert_fail(__FILE__, __LINE__); } while (0)
#else
  #define EMOS_ASSERT(cond)  ((void)0)
#endif
void emos_assert_fail( const char *file, int line );  

/* 函数 */
void emos_init(void);
emos_taskid_t emos_task_register( emos_task_cb_t task_cb,emos_taskid_t  );
void emos_run( void );

void emos_event_post( emos_taskid_t id, emos_event_t events );         
void emos_event_post_after( emos_taskid_t id, emos_event_t events, uint32_t ms );  
void emos_event_post_every( emos_taskid_t id, emos_event_t events, uint32_t ms ); 
void emos_event_cancel( emos_taskid_t id, emos_event_t events );