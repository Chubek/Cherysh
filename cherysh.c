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

#include <readline/readline.h>
#include <readline/history.h>


#define ERR_EXIT(ERR) (fprintf(PAR_ERR_STREAM, ERR), exit(EXIT_FAILURE))

#define STDIN 		fileno(stdin)
#define STDOUT		fileno(stdout)
#define STDERR  	fileno(stderr)

#define PIPE_INR 	PIPEFD_IN[0]
#define PIPE_INW	PIPEFD_IN[1]

#define PIPE_OUTR	PIPEFD_IN[0]
#define PIPE_OUTW	PIPEFD_OUT[1]

#define PIPE_ERRR	PIPEFD_IN[0]
#define PIPE_ERRW	PIPEFD_OUT[1]

#define CHERYSH_PATH	CHERYSH_ARGV[0]

static int CHL_IN_FDESC;
static int CHL_OUT_FDESC;
static int CHL_ERR_FDESC;
static int PAR_IN_FDESC;
static int PAR_OUT_FDESC;
static int PAR_ERR_FDESC;

static int PIPEFD_IN[2];
static int PIPEFD_OUT[2];
static int PIPEFD_ERR[2];

static FILE *CHL_IN_STREAM;
static FILE *CHL_OUT_STREAM;
static FILE *CHL_ERR_STREAM;
static FILE *PAR_IN_STREAM;
static FILE *PAR_OUT_STREAM;
static FILE *PAR_ERR_STREAM;

static uint8_t	PROG_STR[PATH_MAX];

extern uint8_t	**environ;
static uint8_t  *ARGV_ARR[ARG_MAX];
static uint8_t  *ENV_ARR[ARG_MAX];

static size_t ARGC;
static size_t ENVC;

static int WAIT_SIGNAL;
static int EXIT_REASON;

static uint8_t **CHERYSH_ARGV;
static int	 CHERYSH_ARGC;

static pid_t   CHILD_PID;
static int     LAST_EXIT_CODE;

static struct Quote
{
	struct Quote *prev, *next;
	enum QType { Word, Arg, Var, Root, Term,  } type;
	uintptr_t value;
}
QUOTETBL[UINT16_MAX + 1];
static size_t QNUM;
static uintptr_t SYMTBL[UINT16_MAX + 1];
static FILE *SCRIPT_STREAM;
static uint8_t *EVAL_READY_STR;

static inline struct Quote* quote_make(
		struct Quote *quote, enum QType type, uintptr_t value)
{
	struct Quote *old_quote = quote;
	struct Quote *new_quote = (struct Quote*)&QUOTETBL[QNUM++];
	new_quote->prev = old_quote;
	old_quote->next = new_quote;
	new_quote->type = type;
	new_quote->value = value;

	return new_quote;
}

static inline uintptr_t symtable_set(uint8_t *id, uintptr_t value, size_t lenval)
{
	uint8_t chr; uint16_t hash; while ((chr = *id++)) hash = (hash * 33) + chr;
	value && lenval
		? return memmove(&SYMTBL[hash - 1], value, lenval)
		: return SYMTBL[hash - 1];
}

static inline uintptr_t symtable_insert_quote(uint8_t *id, struct Quote *quote)
{    return symtable_set(id, (uintptr_t)quote, sizeof(struct Quote));     }

static inline uintptr_t symtable_insert_int(uint8_t *id, int64_t i)
{    return symtable_set(id, (uintptr_t)i, sizeof(int64_t));  }

static inline uintptr_t symtable_insert_str(uint8_t *id, uint8_t *str)
{    return symtable_set(id, (uintptr_t)str, sizeof(uint8_t*));  }

static inline uintptr_t symtable_insert_float(uint8_t *id, long double flt)
{    return symtable_set(id, (uintptr_t)flt, sizeof(long double));  }

static inline void hook_stdin(FILE **fp, int *fdp) { *fp = stdin; *fdp = STDIN; }
static inline void hook_stdout(FILE **fp, int *fdp) { *fp = stdout; *fdp = STDOUT; }
static inline void hook_stderr(FILE **fp, int *fdp) { *fp = stderr; *fdp = STDERR; }
static inline void hook_stream(FILE *s, FILE **fp, int *fdp)
{ *fp = s; *fdp = fileno(*fp);    }


static inline int hook_fdesc(int fd, FILE **fp, int *fdp, const char *mode)
{ 
	(*fp = fdopen(fd, mode)) < 0 
		? ERR_EXIT("Error opening filedesc\n") 
		: NULL;
       	*fdp = fileno(*fp); 
}

static inline void hook_path(uint8_t *p, FILE **fp, int *fdp, const char *mode)
{ 
	(*fp = fopen(p, mode)) < 0
		? ERR_EXIT("Error opening file\n")
		: NULL; 
	*fdp = fileno(*fp); 
}

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
	while ((int fd = &pfds[0]), *fd++) close(fd);
}


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


void exit_action(void) { close_procio(); close_pario(); free_all(); }


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

static void eval_fragment(void)
{ CheryshYYParse(EVAL_READY_STR); }

