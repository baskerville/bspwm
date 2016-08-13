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

#include <math.h>
#include "types.h"
#include "geometry.h"

bool is_inside(xcb_point_t p, xcb_rectangle_t r)
{
	return (p.x >= r.x && p.x < (r.x + r.width) &&
	        p.y >= r.y && p.y < (r.y + r.height));
}

unsigned int area(xcb_rectangle_t r)
{
	return r.width * r.height;
}


dpoint_t center(xcb_rectangle_t r)
{
	return (dpoint_t) {(double)r.x + ((double)r.width / 2), (double)r.y + ((double)r.height / 2)};
}

double distance_center(xcb_rectangle_t r1, xcb_rectangle_t r2)
{
	dpoint_t r1_center = center(r1);
	dpoint_t r2_center = center(r2);
	return hypot(r1_center.x - r2_center.x, r1_center.y - r2_center.y);
}

bool on_dir_side(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir)
{
	dpoint_t r1_max = {r1.x + r1.width, r1.y + r1.height};
	dpoint_t r2_max = {r2.x + r2.width, r2.y + r2.height};
	dpoint_t r1_center = center(r1);
	dpoint_t r2_center = center(r2);

	switch (dir) {
		case DIR_NORTH:
			if (r2_center.y >= r1_center.y)
				return false;
			break;
		case DIR_WEST:
			if (r2_center.x >= r1_center.x)
				return false;
			break;
		case DIR_SOUTH:
			if (r1_center.y >= r2_center.y)
				return false;
			break;
		case DIR_EAST:
			if (r1_center.x >= r2_center.x)
				return false;
			break;
		default:
			return false;
	}

	switch (dir) {
		case DIR_NORTH:
		case DIR_SOUTH:
			return
				(r2.x >= r1.x && r2.x <= r1_max.x) ||
				(r2_max.x >= r1.x && r2_max.x <= r1_max.x) ||
				(r1.x >= r2.x && r1.x <= r2_max.x) ||
				(r1_max.x >= r2.x && r1_max.x <= r2_max.x);
			break;
		case DIR_WEST:
		case DIR_EAST:
			return
				(r2.y >= r1.y && r2.y <= r1_max.y) ||
				(r2_max.y >= r1.y && r2_max.y <= r1_max.y) ||
				(r1.y >= r2.y && r1.y <= r2_max.y) ||
				(r1_max.y >= r2.y && r1_max.y <= r2_max.y);
			break;
		default:
			return false;
	}
}

bool rect_eq(xcb_rectangle_t a, xcb_rectangle_t b)
{
	return (a.x == b.x && a.y == b.y &&
	        a.width == b.width && a.height == b.height);
}

int rect_cmp(xcb_rectangle_t r1, xcb_rectangle_t r2)
{
	if (r1.y >= (r2.y + r2.height)) {
		return 1;
	} else if (r2.y >= (r1.y + r1.height)) {
		return -1;
	} else {
		if (r1.x >= (r2.x + r2.width)) {
			return 1;
		} else if (r2.x >= (r1.x + r1.width)) {
			return -1;
		} else {
			return area(r1) - area(r2);
		}
	}
}
