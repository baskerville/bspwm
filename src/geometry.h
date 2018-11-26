/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BSPWM_GEOMETRY_H
#define BSPWM_GEOMETRY_H

#include <stdbool.h>
#include <xcb/xcb.h>

bool is_inside(xcb_point_t p, xcb_rectangle_t r);
bool contains(xcb_rectangle_t a, xcb_rectangle_t b);
unsigned int area(xcb_rectangle_t r);
uint32_t boundary_distance(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir);
bool on_dir_side(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir);
bool rect_eq(xcb_rectangle_t a, xcb_rectangle_t b);
int rect_cmp(xcb_rectangle_t r1, xcb_rectangle_t r2);

#endif
