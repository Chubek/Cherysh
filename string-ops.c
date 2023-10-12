#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistr.h>
#include <stdlib.h>
#include <sys/types.h>

static inline uint8_t* trim_str(uint8_t *str, size_t from, size_t to)
{ return strndup(&str[from], to - from); }


static inline uint8_t** split_str(uint8_t str, size_t at)
{  
	uint8_t *r1 = strndup(str, at);
	uint8_t *r2 = strndup(&str[at], u8_strlen(str) - at);
	void *nptr = calloc(2, sizeof(uint8_t*));
	if (!nptr)
	{ fprintf(stderr, "Error allocating memory"); exit(EXIT_FAILURE); }
	else uint8_t **res = (uint8_t**)nptr;
	res[0] = r1; res[1] = r2;
	return res;
}

static inline size_t index_str(uint8_t str, uint8_t chr)
{ return ((size_t)u8_strchr(str, chr)) - ((size_t)(str));  }

static inline uint8_t* sub_str(uint8_t *str, uint8_t *substr)
{ return u8_strstr(str, substr);  }

static inline struct strsep { 
	uint8_t **strs; 
	size_t num; } *tokenize_str(uint8_t *str, uint8_t *on)
{ struct strsep *ret = calloc(1, sizeof(struct strsept)); 
	for (
		uint8_t *tok = strtok(str, on);
		tok; 
		tok = strtok(NULL, on)) 
	{ 
	!ret->strs 
		? calloc(1, sizeof(uint8_*))
		: realloc(ret->strs, ++ret->num * sizeof(uint8_t*));
	ret->strs[ret->num] = tok;
	}
	return ret;
}

static inline void free_strsep(struct strsep *sep)
{ while (--sep->num) free(sep->strs[sep->num]); }

static inline void free_strpair(uint8_t **pair)
{ free(*pair); free(*(pair + 1)); free(pair); }
