/***************************************
 A header file for the ways.

 Part of the Routino routing software.
		      ******************//******************
 This file Copyright 2008-2013 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#ifndef WAYS_H
#define WAYS_H			/*+ To stop multiple inclusions. + */

#include <stdint.h>
#include <stdlib.h>

#include "types.h"

#include "cache.h"
#include "files.h"


/* Data structures */


/*+ A structure containing a single way (members ordered to minimise overall size). +*/
struct _Way {
	index_t name;		/*+ The offset of the name of the way in the names array. + */

	transports_t allow;	/*+ The type of traffic allowed on the way. + */

	highway_t type;		/*+ The highway type of the way. + */

	properties_t props;	/*+ The properties of the way. + */

	speed_t speed;		/*+ The defined maximum speed limit of the way. + */

	weight_t weight;	/*+ The defined maximum weight of traffic on the way. + */
	height_t height;	/*+ The defined maximum height of traffic on the way. + */
	width_t width;		/*+ The defined maximum width of traffic on the way. + */
	length_t length;	/*+ The defined maximum length of traffic on the way. + */
};


/*+ A structure containing the header from the file. +*/
typedef struct _WaysFile {
	index_t number;		/*+ The number of ways. + */

	highways_t highways;	/*+ The types of highways that were seen when parsing. + */
	transports_t allow;	/*+ The types of traffic that were seen when parsing. + */
	properties_t props;	/*+ The properties that were seen when parsing. + */
} WaysFile;


/*+ A structure containing a set of ways (and pointers to mmap file). +*/
struct _Ways {
	WaysFile file;		/*+ The header data from the file. + */

#if !SLIM

	void *data;		/*+ The memory mapped data. + */

	Way *ways;		/*+ An array of ways. + */
	char *names;		/*+ An array of characters containing the names. + */

#else

	int fd;			/*+ The file descriptor for the file. + */
	off_t namesoffset;	/*+ The offset of the names within the file. + */

	Way cached[3];		/*+ Two cached nodes read from the file in slim mode. + */

	char *ncached[3];	/*+ The cached way name. + */

	WayCache *cache;	/*+ A RAM cache of ways read from the file. + */

#endif
};


/* Functions in ways.c */

Ways *LoadWayList(const char *filename);

void DestroyWayList(Ways * ways);

int WaysCompare(Way * way1p, Way * way2p);


/* Macros and inline functions */

#if !SLIM

/*+ Return a Way* pointer given a set of ways and an index. +*/
#define LookupWay(xxx,yyy,zzz)     (&(xxx)->ways[yyy])

/*+ Return the name of a way given the Way pointer and a set of ways. +*/
#define WayName(xxx,yyy)           (&(xxx)->names[(yyy)->name])

#else

static inline Way *LookupWay(Ways * ways, index_t index, int position);

static inline char *WayName(Ways * ways, Way * wayp);

CACHE_NEWCACHE_PROTO(Way)
    CACHE_DELETECACHE_PROTO(Way)
    CACHE_FETCHCACHE_PROTO(Way)
    CACHE_INVALIDATECACHE_PROTO(Way)


/* Inline functions */
    CACHE_STRUCTURE(Way)
    CACHE_NEWCACHE(Way)
    CACHE_DELETECACHE(Way)
    CACHE_FETCHCACHE(Way)
    CACHE_INVALIDATECACHE(Way)


/*++++++++++++++++++++++++++++++++++++++
  Find the Way information for a particular way.

  Way *LookupWay Returns a pointer to the cached way information.

  Ways *ways The set of ways to use.

  index_t index The index of the way.

  int position The position in the cache to store the value.
  ++++++++++++++++++++++++++++++++++++++*/
static inline Way *LookupWay(Ways * ways, index_t index, int position)
{
	ways->cached[position - 1] =
	    *FetchCachedWay(ways->cache, index, ways->fd,
			    sizeof(WaysFile));

	return (&ways->cached[position - 1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the name of a way.

  char *WayName Returns a pointer to the name of the way.

  Ways *ways The set of ways to use.

  Way *wayp The Way pointer.
  ++++++++++++++++++++++++++++++++++++++*/

static inline char *WayName(Ways * ways, Way * wayp)
{
	int position = wayp - &ways->cached[-1];
	int n = 0;

	if (!ways->ncached[position - 1])
		ways->ncached[position - 1] = (char *) malloc(64);

	while (!SlimFetch
	       (ways->fd, ways->ncached[position - 1] + n, 64,
		ways->namesoffset + wayp->name + n)) {
		int i;

		for (i = n; i < n + 64; i++)
			if (ways->ncached[position - 1][i] == 0)
				goto exitloop;

		n += 64;

		ways->ncached[position - 1] =
		    (char *) realloc((void *) ways->ncached[position - 1],
				     n + 64);
	}

      exitloop:

	return (ways->ncached[position - 1]);
}

#endif


#endif				/* WAYS_H */
