/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>

/* FIXME change name and create seperate header */
typedef uint32_t gfp_t;
/* currently ignored */

struct pages {
	addr_t paddr;
	addr_t count;
	gfp_t flags;
}

struct page_allocator {
	struct pages *(*alloc)(addr_t count, gfp_t flags);
	void (*free)(struct pages *pages);
};

int page_allocator_init();
void set_page_allocator(struct page_allocator *allocator);

struct pages * alloc_pages(addr_t count, gfp_t flags);
void free_pages(struct pages *pages);

