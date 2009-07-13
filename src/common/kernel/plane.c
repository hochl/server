/* vi: set ts=2:
 *
 *  
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schr�der
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "plane.h"

/* kernel includes */
#include "region.h"
#include "faction.h"

/* util includes */
#include <util/attrib.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/lists.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct plane *planes;

int plane_width(const plane * pl)
{
  if (pl) {
    return pl->maxx-pl->minx+1;
  }
  return 0;
}

int plane_height(const plane * pl)
{
  if (pl) {
    return pl->maxy-pl->miny+1;
  }
  return 0;
}

static plane * home_plane = NULL;

plane *
get_homeplane(void)
{
  return getplanebyid(0);
}


plane *
getplane(const region *r)
{
  if (r) {
    return r->_plane;
  }
  return get_homeplane();
}

plane *
getplanebyid(int id)
{
  plane *p;

  for (p=planes; p; p=p->next) {
    if (p->id == id) {
      return p;
    }
  }
  return NULL;
}

plane *
getplanebyname(const char * name)
{
  plane *p;

  for (p=planes; p; p=p->next)
    if (!strcmp(p->name, name))
      return p;
  return NULL;
}

plane *
findplane(int x, int y)
{
  plane *pl;

  for(pl=planes;pl;pl=pl->next) {
    if(x >= pl->minx && x <= pl->maxx
        && y >= pl->miny && y <= pl->maxy) {
      return pl;
    }
  }
  return NULL;
}

int
getplaneid(const region *r)

{
  if(r) {
    plane * pl = getplane(r);
    if (pl) return pl->id;

    for(pl=planes;pl;pl=pl->next) {
      if(r->x >= pl->minx && r->x <= pl->maxx
          && r->y >= pl->miny && r->y <= pl->maxy) {
        return pl->id;
      }
    }
  }
  return 0;
}

static int
ursprung_x(const faction *f, const plane *pl, const region * rdefault)
{
  ursprung *ur;
  int id = 0;

  if(!f)
    return 0;

  if(pl)
    id = pl->id;

  for(ur = f->ursprung; ur; ur = ur->next) {
    if(ur->id == id)
      return ur->x;
  }
  if (!rdefault) return 0;
  set_ursprung((faction*)f, id, rdefault->x - plane_center_x(pl), rdefault->y - plane_center_y(pl));
  return rdefault->x - plane_center_x(pl);
}

static int
ursprung_y(const faction *f, const plane *pl, const region * rdefault)
{
  ursprung *ur;
  int id = 0;

  if(!f)
    return 0;

  if(pl)
    id = pl->id;

  for(ur = f->ursprung; ur; ur = ur->next) {
    if(ur->id == id)
      return ur->y;
  }
  if (!rdefault) return 0;
  set_ursprung((faction*)f, id, rdefault->x - plane_center_x(pl), rdefault->y - plane_center_y(pl));
  return rdefault->y - plane_center_y(pl);
}

int 
plane_center_x(const plane *pl)
{
  if(pl == NULL)
    return 0;

  return(pl->minx + pl->maxx)/2;
}

int 
plane_center_y(const plane *pl)
{
  if(pl == NULL)
    return 0;

  return(pl->miny + pl->maxy)/2;
}

int
region_x(const region *r, const faction *f)
{
  plane *pl = rplane(r);
  int x = r->x - plane_center_x(pl);
  if (f) {
    int width = plane_width(pl);
    int width_2 = width/2;
    x -= ursprung_x(f, pl, r);
    if (x>width_2) x -= width;
  }
  return x;
}

int
region_y(const region *r, const faction *f)
{
  plane *pl = rplane(r);
  int y = r->y - plane_center_y(pl);
  if (f) {
    int height = plane_height(pl);
    int height_2 = height/2;
    y -= ursprung_y(f, pl, r);
    if (y>height_2) y -= height;
  }
  return y;
}

void
set_ursprung(faction *f, int id, int x, int y)
{
  ursprung *ur;
  assert(f!=NULL);
  for(ur=f->ursprung;ur;ur=ur->next) {
    if (ur->id == id) {
      ur->x = ur->x + x;
      ur->y = ur->y + y;
      return;
    }
  }

  ur = calloc(1, sizeof(ursprung));
  ur->id   = id;
  ur->x    = x;
  ur->y    = y;

  addlist(&f->ursprung, ur);
}

plane *
create_new_plane(int id, const char *name, int minx, int maxx, int miny, int maxy, int flags)
{
  plane *pl = getplanebyid(id);

  if (pl) return pl;
  pl = calloc(1, sizeof(plane));

  pl->next  = NULL;
  pl->id    = id;
  if (name) pl->name  = strdup(name);
  pl->minx = minx;
  pl->maxx = maxx;
  pl->miny = miny;
  pl->maxy = maxy;
  pl->flags = flags;

  addlist(&planes, pl);
  if (id==0) {
    home_plane = pl;
  }
  return pl;
}

/* Umrechnung Relative-Absolute-Koordinaten */
int 
rel_to_abs(const struct plane *pl, const struct faction * f, int rel, unsigned char index)
{
  assert(index == 0 || index == 1);

  if(index == 0)
    return (rel + ursprung_x(f, pl, NULL) + plane_center_x(pl));

  return (rel + ursprung_y(f, pl, NULL) + plane_center_y(pl));
}


int
resolve_plane(variant id, void * addr)
{
  int result = 0;
  plane * pl = NULL;
  if (id.i!=0) {
    pl = getplanebyid(id.i);
    if (pl==NULL) {
      result = -1;
    }
  }
  *(plane**)addr = pl;
  return result;
}

void
write_plane_reference(const plane * u, struct storage * store)
{
  store->w_int(store, u?(u->id):0);
}

int
read_plane_reference(plane ** pp, struct storage * store)
{
  variant id;
  id.i = store->r_int(store);
  if (id.i==0) {
    *pp = NULL;
    return AT_READ_FAIL;
  }
  *pp = getplanebyid(id.i);
  if (*pp==NULL) ur_add(id, pp, resolve_plane);
  return AT_READ_OK;
}

boolean
is_watcher(const struct plane * p, const struct faction * f)
{
  struct watcher * w;
  if (!p) return false;
  w = p->watchers;
  while (w && w->faction!=f) w=w->next;
  return (w!=NULL);
}
