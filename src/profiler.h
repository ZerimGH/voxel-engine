#ifndef PROFILER_H

#define PROFILER_H

#include <stdio.h>
#include <time.h>

#define START_TIMER(name) clock_t __start_##name = clock();

#define END_TIMER(name)                                                        \
  do {                                                                         \
    clock_t __end_##name = clock();                                            \
    double __elapsed_##name =                                                  \
        (double)(__end_##name - __start_##name) * 1000.0 / CLOCKS_PER_SEC;     \
    printf("[%s] Elapsed: %.3f ms\n", #name, __elapsed_##name);                \
  } while (0)

#endif
