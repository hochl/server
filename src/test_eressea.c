#include <platform.h>
#include <kernel/config.h>
#include <CuTest.h>
#include <stdio.h>
#include <util/log.h>


#define ADD_TESTS(suite, name) \
CuSuite *get_##name##_suite(void); \
CuSuiteAddSuite(suite, get_##name##_suite())

int RunAllTests(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();
  int flags = log_flags;

  log_flags = LOG_FLUSH | LOG_CPERROR;
  kernel_init();

  /* self-test */
  ADD_TESTS(suite, tests);
  ADD_TESTS(suite, callback);
  ADD_TESTS(suite, json);
  ADD_TESTS(suite, jsonconf);
  ADD_TESTS(suite, direction);
  ADD_TESTS(suite, skill);
  ADD_TESTS(suite, keyword);
  ADD_TESTS(suite, order);
  /* util */
  ADD_TESTS(suite, config);
  ADD_TESTS(suite, base36);
  ADD_TESTS(suite, bsdstring);
  ADD_TESTS(suite, functions);
  ADD_TESTS(suite, umlaut);
  /* kernel */
  ADD_TESTS(suite, faction);
  ADD_TESTS(suite, build);
  ADD_TESTS(suite, pool);
  ADD_TESTS(suite, curse);
  ADD_TESTS(suite, equipment);
  ADD_TESTS(suite, item);
  ADD_TESTS(suite, magic);
  ADD_TESTS(suite, move);
  ADD_TESTS(suite, reports);
  ADD_TESTS(suite, save);
  ADD_TESTS(suite, ship);
  ADD_TESTS(suite, spellbook);
  ADD_TESTS(suite, building);
  ADD_TESTS(suite, spell);
  ADD_TESTS(suite, battle);
  ADD_TESTS(suite, ally);
  /* gamecode */
  ADD_TESTS(suite, market);
  ADD_TESTS(suite, laws);
  ADD_TESTS(suite, economy);

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);

  log_flags = flags;
  return suite->failCount;
}

int main(int argc, char ** argv) {
    log_stderr = 0;
    return RunAllTests();
}
