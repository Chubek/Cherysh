#if __STDC_VERSION__ < 201112L || __STDC_NO_ATOMICS__ == 1
#error "Your compiler does not support atomics"
#endif

#if __STDC_VERSION__ < 201112L || __STDC_NO_THREADS__ == 1
#error "Your compiler does not support threads"
#endif


#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <threads.h>
#include <stdatomic.h>

#ifndef MAX_PARALLEL_PROC
#define MAX_PARALLEL_PROC 64
#endif

volatile atomic_int PROCS_FDESC_IN[MAX_PARALLEL_PROC];
volatile atomic_int PROCS_FDESC_OUT[MAX_PARALLEL_PROC];
volatile atomic_int PROCS_FDESC_ERR[MAX_PARALLEL_PROC];

volatile atomic_uintptr_t PROCS_IN_STREAM[MAX_PARALLEL_PROC];
volatile atomic_uintptr_t PROCS_OUT_STREAM[MAX_PARALLEL_PROC];
volatile atomic_uintptr_t PROCS_ERR_STREAM[MAX_PARALLEL_PROC];

volatile atomic_uchar 	PROG_PATH[MAX_PATH][MAX_PARALLEL_PROC];
volatile atomic_uchar 	PROG_ARGV[MAX_ARG][MAX_PARALLEL_PROC];
volatile atomic_size_t 	PROG_ARGC[MAX_PARALLEL_PROC];

volatile atomic_int		PROCS_PIDS[MAX_PARALLEL_PROC];
volatile atomic_uintptr_t	PROCS_INITS[MAX_PARALLEL_PROC];
volatile atomic_uchar		PROCS_EXECORDER[MAX_PARALLEL_PROC];

static size_t NUM_DEF_PROCS;
static size_t SUBJ_DEF_PROC;



