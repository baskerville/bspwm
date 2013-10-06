#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "window.h"
#include "history.h"
#include "tree.h"
#include "tag.h"

tag_t *make_tag(char *name, int idx)
{
    tag_t *tag = malloc(sizeof(tag_t));
    if (tag != NULL) {
        snprintf(tag->name, sizeof(tag->name), "%s", name);
        tag->mask = 1 << idx;
    }
    return tag;
}

bool add_tag(char *name)
{
    if (num_tags >= MAXTAGS)
        return false;
    tag_t *tag = make_tag(name, num_tags);
    if (tag == NULL)
        return false;
    tags[num_tags] = tag;
    num_tags++;
    return true;
}

bool remove_tag(char *name)
{
    for (int i = 0; i < num_tags; i++)
        if (streq(tags[i]->name, name))
            return remove_tag_by_index(i);
    return false;
}

bool remove_tag_by_index(int i)
{
    if (i >= num_tags)
        return false;
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
            tag_desktop(m, d, d->tags_field & ~tags[i]->mask);
            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                tag_node(m, d, n, d, n->client->tags_field & ~tags[i]->mask);
        }
    free(tags[i]);
    for (int j = i; j < (num_tags - 1); j++)
        tags[j] = tags[j + 1];
    tags[num_tags - 1] = NULL;
    num_tags--;
    return true;
}

tag_t *get_tag(char *name)
{
    for (int i = 0; i < num_tags; i++)
        if (streq(tags[i]->name, name))
            return tags[i];
    return NULL;
}

tag_t *get_tag_by_index(int i)
{
    if (i >= num_tags)
        return NULL;
    return tags[i];
}

void set_visibility(monitor_t *m, desktop_t *d, node_t *n, bool visible)
{
    PRINTF("set visibilty %X: %s\n", n->client->window, BOOLSTR(visible));

    if (!n->client->floating) {
        n->vacant = !visible;
        update_vacant_state(n->parent);
        if (visible)
            rotate_brother(n);
        else
            unrotate_brother(n);
    }
    if (visible) {
        if (m->desk == d)
            window_show(n->client->window);
        if (d->focus == NULL) {
            if (mon->desk == d)
                focus_node(m, d, n);
            else
                pseudo_focus(d, n);
        }
    } else {
        if (m->desk == d)
            window_hide(n->client->window);
        if (d->focus == n) {
            node_t *f = history_get_node(d, n);
            if (f == NULL)
                f = closest_visible(d, n);
            if (mon->desk == d)
                focus_node(m, d, f);
            else
                pseudo_focus(d, f);
        }
    }
}

void set_presence(monitor_t *m, desktop_t *d, node_t *n, bool present)
{
    if (is_visible(d, n) != present) {
        if (present)
            tag_node(m, d, n, d, n->client->tags_field | d->tags_field);
        else
            tag_node(m, d, n, d, n->client->tags_field & ~d->tags_field);
    }
}

void tag_node(monitor_t *m, desktop_t *d, node_t *n, desktop_t *ds, unsigned int tags_field)
{
    if (num_tags < 1)
        return;
    bool visible = is_visible(ds, n);
    n->client->tags_field = tags_field;
    if ((visible && (tags_field & d->tags_field) == 0)
            || (!visible && (tags_field & d->tags_field) != 0)) {
        set_visibility(m, d, n, !visible);
        arrange(m, d);
    }
}

void tag_desktop(monitor_t *m, desktop_t *d, unsigned int tags_field)
{
    if (num_tags < 1)
        return;
    bool dirty = false;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
        bool visible = is_visible(d, n);
        if ((visible && (tags_field & n->client->tags_field) == 0)
                || (!visible && (tags_field & n->client->tags_field) != 0)) {
            set_visibility(m, d, n, !visible);
            dirty = true;
        }
    }
    d->tags_field = tags_field;
    if (dirty)
        arrange(m, d);
    if (d == mon->desk)
        put_status();
}

void list_tags(char *rsp)
{
    char line[MAXLEN];
    for (int i = 0; i < num_tags; i++) {
        snprintf(line, sizeof(line), "%s %u\n", tags[i]->name, tags[i]->mask);
        strncat(rsp, line, REMLEN(rsp));
    }
}

void init_tags(void)
{
    num_tags = 0;
    for (int i = 0; i < MAXTAGS; i++)
        tags[i] = NULL;
    add_tag(DEFAULT_TAG_NAME);
}
