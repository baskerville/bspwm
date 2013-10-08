/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BSPWM_TAG_H
#define BSPWM_TAG_H

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
