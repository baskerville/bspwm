#ifndef BSPWM_SUBSCRIBE_H
#define BSPWM_SUBSCRIBE_H

subscriber_list_t *make_subscriber_list(int fd);
void remove_subscriber(subscriber_list_t *sb);
void add_subscriber(int fd);
void feed_subscriber(subscriber_list_t *sb);

#endif
