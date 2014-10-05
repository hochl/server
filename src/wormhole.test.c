#include <platform.h>
#include <kernel/types.h>

#include "wormhole.h"
#include "tests.h"

#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/terrain.h>

#include <util/attrib.h>

#include <quicklist.h>

#include <CuTest.h>

void sort_wormhole_regions(quicklist *rlist, region **match, int count);
void make_wormholes(region **match, int count, const building_type *bt_wormhole);

static void test_make_wormholes(CuTest *tc) {
    region *r1, *r2, *match[2];
    terrain_type *t_plain;
    building_type *btype;

    test_cleanup();
    t_plain = test_create_terrain("plain", LAND_REGION);
    btype = test_create_buildingtype("wormhole");
    match[0] = r1 = test_create_region(0, 0, t_plain);
    match[1] = r2 = test_create_region(1, 0, t_plain);
    make_wormholes(match, 2, btype);
    CuAssertPtrNotNull(tc, r1->buildings);
    CuAssertPtrNotNull(tc, r1->buildings->attribs);
    CuAssertPtrEquals(tc, (void *)r1->buildings->type, (void *)btype);
    CuAssertPtrNotNull(tc, r2->buildings);
    CuAssertPtrNotNull(tc, r2->buildings->attribs);
    CuAssertPtrEquals(tc, (void *)r2->buildings->type, (void *)btype);
    CuAssertPtrEquals(tc, (void *)r1->buildings->attribs->type, (void *)r2->buildings->attribs->type);
    CuAssertStrEquals(tc, r1->buildings->attribs->type->name, "wormhole");
    test_cleanup();
}

static void test_sort_wormhole_regions(CuTest *tc) {
    region *r1, *r2, *match[2];
    terrain_type *t_plain;
    quicklist *rlist = 0;

    test_cleanup();
    t_plain = test_create_terrain("plain", LAND_REGION);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_plain);
    r1->age = 4;
    r2->age = 2;
    ql_push(&rlist, r1);
    ql_push(&rlist, r2);
    sort_wormhole_regions(rlist, match, 2);
    CuAssertPtrEquals(tc, r2, match[0]);
    CuAssertPtrEquals(tc, r1, match[1]);
    ql_free(rlist);
    test_cleanup();
}

CuSuite *get_wormhole_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_sort_wormhole_regions);
    SUITE_ADD_TEST(suite, test_make_wormholes);
    return suite;
}
