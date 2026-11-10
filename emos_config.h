#ifndef EMOS_CONFIG_H
#define EMOS_CONFIG_H

#ifndef EMOS_TASK_MAX
#define EMOS_TASK_MAX                     10
#endif

#ifndef EMOS_DELAY_EVENTS_MAX
#define EMOS_DELAY_EVENTS_MAX             10
#endif

#ifndef EMOS_MSG_LEN_MAX
#define EMOS_MSG_LEN_MAX                  10
#endif

#ifndef EMOS_MEM_POOL_BLK_MAX
#define EMOS_MEM_POOL_BLK_MAX             20
#endif

#ifndef EMOS_MEM_POOL_USER_BLK_CNT
#define EMOS_MEM_POOL_USER_BLK_CNT        20
#endif

#ifndef EMOS_MEM_POOL_USER_BLK_SIZE
#define EMOS_MEM_POOL_USER_BLK_SIZE       128
#endif

#define EMOS_ENTER_CRITICAL()  
#define EMOS_EXIT_CRITICAL()  

#if !defined(EMOS_ASSERT_DISABLE)
  #define EMOS_ASSERT(cond)  do { if (!(cond)) emos_assert_fail(__FILE__, __LINE__); } while (0)
#else
  #define EMOS_ASSERT(cond)  ((void)0)
#endif
void emos_assert_fail( const char *file, int line );  

#endif /* EMOS_CONFIG_H */
