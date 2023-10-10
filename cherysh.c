#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ERR_EXIT(ERR) (fprintf(SPEC_ERR_STREAM, ERR), exit(1))

#define STDIN 	fileno(stdin)
#define STDOUT	fileno(stdout)
#define STDERR  fileno(stderr)

#define PIPE_INR 	PIPEFD_IN[0]
#define PIPE_INW	PIPEFD_IN[1]

#define PIPE_OUTR	PIPEFD_IN[0]
#define PIPE_OUTW	PIPEFD_OUT[1]

#define PIPE_ERRR	PIPEFD_IN[0]
#define PIPE_ERRW	PIPEFD_OUT[1]

#define DOQUSH_PATH	DOQUSH_ARGV[0]

static int IN_FDESC;
static int OUT_FDESC;
static int ERR_FDESC;

static int PIPEFD_IN[2];
static int PIPEFD_OUT[2];
static int PIPEFD_ERR[2];

static FILE *PROC_IN_STREAM;
static FILE *PROC_OUT_STREAM;
static FILE *PROC_ERR_STREAM;
static FILE *SPEC_IN_STREAM;
static FILE *SPEC_OUT_STREAM;
static FILE *SPEC_ERR_STREAM;

static uint8_t	PROG_STR[PATH_MAX];

extern uint8_t	**environ;
static uint8_t  *ARGV_ARR[ARG_MAX];
static uint8_t  *ENV_ARR[ARG_MAX];

static size_t ARGC;
static size_t ENVC;

static int WAIT_SIGNAL;
static int EXIT_REASON;

static uint8_t **DOQUSH_ARGV;
static int	 DOQUSH_ARGC;

static pid_t   CHILD_PID;
static int     LAST_EXIT_CODE;

static uintptr_t SYMTBL[UINT16_MAX + 1];

static inline void open_pipes(void)
{
  pipe(PIPEFD_IN) < 0 ? return -1 : 0; 
  pipe(PIPEFD_OUT) < 0 ? return -1 : 0; 
  pipe(PIPEFD_ERR) < 0 ? return -1 : 0; 
}

static inline void close_pipes(void)
{ 
	int pfds[7] = { 0,
		PIPE_INR, PIPE_INW, PIPE_OUTR, PIPE_OUTW, PIPE_ERRR, PIPE_ERRW, 
	};
	while ((int j = &pfds[0]), *j++) close(j);
}

static inline void setin_std(void) 
{ IN_FDESC = STDIN; PROC_IN_STREAM = stdin; }

static inline void setout_std(void) 
{ OUT_FDESC = STDOUT; PROC_OUT_STREAM = stdout; }

static inline void seterr_std(void) 
i{ ERR_FDESC = STDERR; PROC_ERR_STREAM = stderr; }


static inline void setin_fd(int fd) 
{ IN_FDESC = fd; PROC_IN_STREAM = fdopen(fd, "rw"); }

static inline void setout_fd(int fd) 
{ OUT_FDESC = fd; PROC_OUT_STREAM = fdopen(fd, "rw"); }

static inline void seterr_fd(int fd) 
{ ERR_FDESC = fd; PROC_ERR_STREAM = fdopen(fd, "rw"); }


static inline void setin_path(unsinged char *p) 
{ PROC_IN_STREAM = fopen(p, "rw"); IN_FDESC = fileno(PROC_IN_STREAM); }

static inline void setout_path(unsigned char *p) 
{ PROC_OUT_STREAM = fopen(p, "rw"); OUT_FDESC = fileno(PROC_OUT_STREAM); }

static inline void seterr_path(unsigned char *p) 
{ PROC_ERR_STREAM = fopen(p, "rw"); ERR_FDESC = fileno(PROC_ERR_STREAM); }

static inline void open_specin_stdin(void)
{ SPEC_IN_STREAM = stdin;   }

static inline void open_specout_stdout(void)
{ SPEC_IN_STREAM = stdout;   }

static inline void open_specerr_stderr(void)
{ SPEC_IN_STREAM = stderr;   }

static inline void open_specin_path(uint8_t *p)
{ SPEC_IN_STREAM = fopen(p, "r");   }

static inline void open_specout_path(uint8_t *p)
{ SPEC_OUT_STREAM = fopen(p, "w");   }

static inline void open_specerr_path(uint8_t *p)
{ SPEC_ERR_STREAM = fopen(p, "w");   }


static inline void close_procio(void) 
{ fclose(PROC_IN_STREAM); fclose(PROC_OUT_STREAM); fclose(PROC_ERR_STREAM); }

static inline void close_specio(void)
{ fclose(SPEC_IN_STREAM); fclose(SPEC_OUT_STREAM); fclose(SPEC_ERR_STREAM); }

static inline void setsig_wait(int sig)
{ WAIT_SIGNAL = sig;   }

static inline void set_prog(uint8_t *prog)
{  !prog 
	? memset(&PROG_STR[0], 0, PATH_MAX) 
        : strncpy(&PROG_STR[0], 0, PATH_MAX);
}

static inline void set_environ_dfl(void) { ENV_ARR = environ; }

static inline void set_argv(uint8_t *arg)
{
	ARGC >= ARG_MAX ? ERR_EXIT("Argument overflow\n") : NULL;
	!arg
	  ? for ((arg = &ARGV_ARR[0]; arg; free(arg), *arg++))
	  : (ARGV_ARR[ARGC++] = strdup(arg)); 
}

static inline void set_environ(uint8_t *env)
{
	ENVC >= ARG_MAX ? ERR_EXIT("Environ overflow\n") : NULL;
	!env
	  ? for ((env = &ENV_ARR[0]; env; free(env), *env++))
	  : (ENV_ARR[ENVC++] = strdup(env)); 
}

static inline void free_all(void)
{ set_argv(NULL); set_environ(NULL); }


void exit_action(void) { close_procio(); close_specio(); free_all(); }


void signal_handler(int signum)
{ 
	if (signum == WAIT_SIGNAL) 
	{  
		exit(EXIT_REASON); CHILD_PID ? kill(CHILD_PID, SIGINT) : NULL;  
	}  
}

static void handle_child(void)
{
	close(PIPE_INW); close(PIPE_OUTR); close(PIPE_ERRR);
	
	int indup, outdup, errdup;
	indup 	= dup(IN_FDESC);
	outdup	= dup(OUT_FDESC);
	errdup  = dup(ERR_FDESC);

	dup2(PIPE_INR, indup); dup2(PIPE_OUTW, outdup); dup2(PIPE_ERRW, errdup);

	execvpe(PROG_STR, ARGV_ARR, ENV_ARR) < 0
		? ERR_EXIT("Error executing process\n")
		: NULL;

	close(indup); close(outdup); close(errdup);
}

static void handle_parent(void)
{
	close(PIPE_INR); close(PIPE_OUTW); close(PIPE_ERRW);

	waitpid(CHILD_PID, &LAST_EXIT_CODE, WNOHAND | WUNTRACED) < 0
		? ERR_STR("Error waiting for process to finish\n")
		: NULL;

}

static void execute_program(void)
{
	if (WAIT_SIGNAL > 0) signal(WAIT_SIGNAL, signal_handler);
	
	open_pipes() < 0
 		? ERR_EXIT("Error opening pipes\n")
		: NULL;

	(CHILD_PID = fork()) < 0
		? ERR_EXIT("Error forking process\n")
		: !CHILD_PID 
			? handle_child()
			: handle_parent();
	exec_callback();
	close_pipes();
	exec_cleanup();

}

