#ifndef __YAHOO_UTIL_H__
#define __YAHOO_UTIL_H__

#include <stdlib.h>

#ifndef FREE
#define FREE(x)	if(x) {free(x); x=NULL;}
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#define y_new(type, n)		(type *)malloc(sizeof(type) * (n))
#define y_new0(type, n)		(type *)calloc((n), sizeof(type))
#define y_renew(type, mem, n)	(type *)realloc(mem, n)

void * y_memdup(void * addr, int n);
char ** y_strsplit(char * str, char * sep, int nelem);
void y_strfreev(char ** vector);

char * y_string_append(char * str, char * append);

#endif
