#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include "yahoo_util.h"

char * y_string_append(char * string, char * append)
{
	int size = strlen(string) + strlen(append) + 1;
	char * new_string = y_renew(char, string, size);

	if(new_string == NULL) {
		new_string = y_new(char, size);
		strcpy(new_string, string);
		FREE(string);
	}

	strcat(new_string, append);

	return new_string;
}

void y_strfreev(char ** vector)
{
	char **v;
	for(v = vector; *v; v++) {
		FREE(*v);
	}
	FREE(vector);
}

char ** y_strsplit(char * str, char * sep, int nelem)
{
	char ** vector;
	char *s, *p;
	int i=0;
	if(nelem < 0) {
		char * s;
		nelem=0;
		for(s=strstr(str, sep); s; s=strstr(s, sep), nelem++)
			;
	}

	vector = y_new(char *, nelem + 1);

	for(p=str, s=strstr(p, sep); i<nelem && s; p=s, s=strstr(p, sep), i++) {
		int len = s-p;
		vector[i] = y_new(char, len+1);
		strncpy(vector[i], p, len);
		vector[i][len] = '\0';
	}

	vector[i] = NULL;

	return vector;
}

void * y_memdup(void * addr, int n)
{
	void * new_chunk = y_new(void, n);
	return memcpy(new_chunk, addr, n);
}

