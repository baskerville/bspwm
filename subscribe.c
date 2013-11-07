#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "bspwm.h"
#include "tree.h"
#include "settings.h"
#include "subscribe.h"

subscriber_list_t *make_subscriber_list(int fd)
{
    subscriber_list_t *sb = malloc(sizeof(subscriber_list_t));
    sb->prev = sb->next = NULL;
    sb->fd = fd;
    sb->stream = fdopen(fd, "w");
    if (sb->stream == NULL) {
        warn("Can't open subscriber %i\n", fd);
        close(fd);
        free(sb);
        return NULL;
    }
    return sb;
}

void remove_subscriber(subscriber_list_t *sb)
{
    if (sb == NULL)
        return;
    subscriber_list_t *a = sb->prev;
    subscriber_list_t *b = sb->next;
    if (a != NULL)
        a->next = b;
    if (b != NULL)
        b->prev = a;
    if (sb == subscribe_head)
        subscribe_head = b;
    if (sb == subscribe_tail)
        subscribe_tail = a;
    fclose(sb->stream);
    free(sb);
}

void add_subscriber(int fd)
{
    subscriber_list_t *sb = make_subscriber_list(fd);
    if (sb == NULL)
        return;
    if (subscribe_head == NULL) {
        subscribe_head = subscribe_tail = sb;
    } else {
        subscribe_tail->next = sb;
        sb->prev = subscribe_tail;
        subscribe_tail = sb;
    }
    feed_subscriber(sb);
}

void feed_subscriber(subscriber_list_t *sb)
{
    fprintf(sb->stream, "%s", status_prefix);
    bool urgent = false;
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        fprintf(sb->stream, "%c%s:", (mon == m ? 'M' : 'm'), m->name);
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
            for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n, d->root))
                urgent |= n->client->urgent;
            char c = (urgent ? 'u' : (d->root == NULL ? 'f' : 'o'));
            if (m->desk == d)
                c = toupper(c);
            fprintf(sb->stream, "%c%s:", c, d->name);
        }
    }
    if (mon != NULL && mon->desk != NULL)
        fprintf(sb->stream, "L%s", (mon->desk->layout == LAYOUT_TILED ? "tiled" : "monocle"));
    fprintf(sb->stream, "%s", "\n");
    int ret = fflush(sb->stream);
    if (ret != 0)
        remove_subscriber(sb);
}
