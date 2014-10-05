#include <CuTest.h>
#include "base36.h"
#include <stdlib.h>
#include <limits.h>

static void test_atoi36(CuTest * tc)
{
  CuAssertIntEquals(tc, 0, atoi36("0"));
  CuAssertIntEquals(tc, 666, atoi36("ii"));
  CuAssertIntEquals(tc, -10, atoi36("-a"));
  CuAssertIntEquals(tc, -1, atoi36("-1"));
  CuAssertIntEquals(tc, -10, atoi("-10"));
  CuAssertIntEquals(tc, -10, atoi("-10"));
}

static void test_itoa36(CuTest * tc)
{
  CuAssertStrEquals(tc, itoa36(0), "0");
  CuAssertStrEquals(tc, itoa10(INT_MAX), "2147483647");
  CuAssertStrEquals(tc, itoab(INT_MAX, 8), "17777777777");
  CuAssertStrEquals(tc, itoab(INT_MAX, 4), "1333333333333333");
  CuAssertStrEquals(tc, itoab(-1, 5), "-1");
  CuAssertStrEquals(tc, itoa36(-1), "-1");
  CuAssertStrEquals(tc, itoa36(-10), "-a");
  CuAssertStrEquals(tc, itoa36(666), "ii");
}

CuSuite *get_base36_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_itoa36);
  SUITE_ADD_TEST(suite, test_atoi36);
  return suite;
}
