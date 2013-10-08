#ifndef TAG_H
#define TAG_H

#define MAXTAGS           32
#define DEFAULT_TAG_NAME  "*"

typedef struct {
    char name[SMALEN];
    unsigned int mask;
} tag_t;

tag_t *tags[MAXTAGS];
int num_tags;

tag_t *make_tag(char *name, int idx);
bool add_tag(char *name);
bool remove_tag(char *name);
bool remove_tag_by_index(int i);
tag_t *get_tag(char *name);
tag_t *get_tag_by_index(int i);
void set_visibility(monitor_t *m, desktop_t *d, node_t *n, bool visible);
void set_presence(monitor_t *m, desktop_t *d, node_t *n, bool present);
void tag_node(monitor_t *m, desktop_t *d, node_t *n, desktop_t *ds, unsigned int tags_field);
void tag_desktop(monitor_t *m, desktop_t *d, unsigned int tags_field);
void list_tags(char *rsp);
void init_tags(void);

#endif
