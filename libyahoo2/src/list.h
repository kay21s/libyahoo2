// list.h

#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _YList {
  struct _YList * next;
  struct _YList * prev;
  void * data;
} YList;

typedef int (*YListCompFunc)(const void *, const void *);

YList * y_list_append(YList * list, void * data);
YList * y_list_remove_link(YList * list, YList * link);
YList * y_list_remove(YList * list, void * data);

YList * y_list_copy(YList * list);

void y_list_free_1(YList * list);
void y_list_free(YList * list);
int y_list_length(YList * list);

YList * y_list_find_custom(YList * list, void * data, YListCompFunc comp);

#ifdef __cplusplus
}
#endif

#endif // __LIST_H__
