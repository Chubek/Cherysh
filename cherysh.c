#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistr.h>
#include <unistdio.h>
#include <gc.h>

#define LOCAL 	static
#define INLINE 	static inline

extern char **environ

LOCAL FILE 	 *CURR_IN_STREAM;
LOCAL FILE       *CURR_OUT_STREAM;
LOCAL FILE       *CURR_ERR_STREAM;

LOCAL FILE       *NEXT_IN_STREAM;
LOCAL FILE 	 *NEXT_OUT_STREAM;
LOCAL FILE 	 *NEXT_ERR_STREAM;

LOCAL uint8_t     CURR_STR[CURR_STR_MAX];

LOCAL uint8_t     CURR_PROG[MAX_PATH];
LOCAL uint8_t     *CURR_ARGV[MAX_ARG];
LOCAL size_t	  CURR_ARGC;

LOCAL int	  PIPE_IN[2];
LOCAL int 	  PIEP_OUT[2];
LOCAL int	  PIPE_ERR[2];

LOCAL int	  LAST_EXIT_STAT:

LOCAL uint8_t	 **SHELL_ARGV;
LOCAL size_t	 SHELL_ARGC;
LOCAL uint8_t	 ENV = environ;
LOCAL uint8_t	 HOME[MAX_PATH + 1];
LOCAL uint8_t    LINENO[INT8_MAX + 1];
LOCAL uint8_t	 PATH[MAX_PATH + 1];
LOCAL uint8_t    PS1[UINT8_MAX + 1], PS2[UINT8_MAX + 1], PS4[UINT8_MAX + 1];
LOCAL uint8_t    PWD[MAX_PATH + 1];








INLINE uint8_t *gc_strndup(uint8_t *s, size_t l)
{ uint8_t *d = (uint8_t*)GC_MALLOC(l + 1); d[l] = 0; u8_strncpy(d, s, l); return d; }

INLINE void open_file_stdin(FILE **s)  { *s = stdin; }
INLINE void open_file_stdout(FILE **s) { *s = stdout; }
INLINE void open_file_stderr(FILE **s) { *s = stderr; }

INLINE void open_file_fdesc(FILE **s, 
		int d, const char *m) { *s = fdopen(d, m); }

INLINE void swap_streams(FILE **s1, FILE **s2)
{ FILE *s3 = *s1; *s1 = *s2; *s2 = s3;   }




INLINE void init_argv(void)
{ uint8_t *arg; while ((arg = CURR_ARGV[--CURR_ARGC])) arg = NULL; }

INLINE void add_argv(uint8_t *arg, size_t len)
{ CURR_ARGV[CURR_ARGC++] = gc_strndup(arg, len); }

INLINE void blank_prog(void)
{ memset(&CURR_PROG[0], 0, MAX_PATH);  }

INLINE void set_prog(uint8_t *p, size_t len)
{ u8_strncpy(&CURR_PROG[0], &p[0], len); }







#define IN_R PIPE_IN[0]
#define IN_W PIPE_IN[1]

#define OUT_R PIPE_IN[0]
#define OUT_W PIPE_IN[1]

#define ERR_R PIPE_ERR[0]
#define ERR_W PIPE_ERR[1]

#define STDIN 	fileno(stdin)
#define STDOUT 	fileno(stdout)
#define STDERR  fileno(stderr)


LOCAL void exec_program(void)
{
	pipe(PIPE_IN); pipe(PIPE_OUT); pipe(PIPE_ERR);

	pid_t cpid = fork(), retpid;

	if (!cpid)
	{

		close(IN_W); close(OUT_R); close(ERR_R);
		int indup, outdup, errdup;
		indup  = dup(STDIN);
		outdup = dup(STDOUT);
		errdup = dup(STDERR);

		dup2(indup, IN_R); dup2(outdup, OUT_W); dup2(errdup, ERR_W);
		
		if (execvpe(&CURR_PROG[0], CURR_ARGV, environ) < 0)
		{
			fprintf(stderr, "Error executing program");
			exit(EXIT_FAILURE);
		}
		
		close(IN_R); close(OUT_W); close(ERR_W);
		close(indup); close(outdup); close(errdup);
	}
	else
	{
		close(IN_R); close(OUT_W); close(ERR_W);
		int indup, outdup, errdup;
		indup  = dup(STDIN);
		outdup = dup(STDOUT);
		errdup = dup(STDERR);

		dup2(indup, IN_W); dup2(outdup, OUT_R); dup2(errdup, ERR_R);

		retpid = waitpid(cpid, &LAST_EXIT_STAT, WNOHANG | WUNTRACED);
		if (retpid == pid)
			fprintf(stdout, "[%d] %d", LAST_EXIT_STAT, retpid);

		close(IN_W); close(OUT_R); close(ERR_R);
		close(indup); close(outdup); close(errdup);
	}
}


INLINE void set_pwd(void)
{ getcwd(&PWD[0], MAX_PATH); }

INLINE uint8_t *get_pwd(void)
{ return &PWD[0]; }

INLINE void set_ps1(uint8_t *ps1)
{ u8_strncpy(&PS1[0], ps1, UINT8_MAX); }

INLINE uint8_t *get_ps1(void)
{ return &PS1[0]; }


INLINE void set_ps1(uint8_t *ps1)
{ u8_strncpy(&PS1[0], ps1, UINT8_MAX); }

INLINE uint8_t *get_ps1(void)
{ return &PS1[0]; }


INLINE void set_ps2(uint8_t *ps2)
{ u8_strncpy(&PS2[0], ps2, UINT8_MAX); }

INLINE uint8_t *get_ps2(void)
{ return &PS2[0]; }


INLINE void set_ps4(uint8_t *ps4)
{ u8_strncpy(&PS4[0], ps4, UINT8_MAX); }

INLINE uint8_t *get_ps4(void)
{ return &PS4[0]; }













