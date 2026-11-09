#ifndef EMOS_CONFIG_H
#define EMOS_CONFIG_H

#ifndef EMOS_TASK_MAX
#define EMOS_TASK_MAX               10
#endif

#ifndef EMOS_DELAY_EVENTS_MAX
#define EMOS_DELAY_EVENTS_MAX       10
#endif

/* #define EMOS_DEBUG */

/* 断言*/
#if defined(EMOS_DEBUG)
  #define EMOS_ASSERT(cond)  do { if (!(cond)) emos_assert_fail(__FILE__, __LINE__); } while (0)
#else
  #define EMOS_ASSERT(cond)  ((void)0)
#endif
void emos_assert_fail( const char *file, int line );  

#endif /* EMOS_CONFIG_H */
