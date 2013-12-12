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

#ifndef BSPWM_RULE_H
#define BSPWM_RULE_H

#define MATCH_ANY  "*"
#define CSQ_BLK    " =,\n"

rule_t *make_rule(void);
void add_rule(rule_t *r);
void remove_rule(rule_t *r);
void remove_rule_by_cause(char *cause);
bool remove_rule_by_index(int idx);
rule_consequence_t *make_rule_conquence(void);
pending_rule_t *make_pending_rule(int fd, xcb_window_t win, rule_consequence_t *csq);
void add_pending_rule(pending_rule_t *pr);
void remove_pending_rule(pending_rule_t *pr);
void apply_rules(xcb_window_t win, rule_consequence_t *csq);
bool schedule_rules(xcb_window_t win, rule_consequence_t *csq);
void parse_rule_consequence(int fd, rule_consequence_t *csq);
void parse_key_value(char *key, char *value, rule_consequence_t *csq);
void list_rules(char *pattern, char *rsp);

#endif
