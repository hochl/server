#include <platform.h>

#include <kernel/types.h>
#include <kernel/magic.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <quicklist.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>

int count_spell_cb(spellbook_entry * sbe, void * ptr)
{
  int * counter = (int *)ptr;
  ++*counter;
  return 0;
}

void test_named_spellbooks(CuTest * tc)
{
  spell *sp;
  spellbook *sb;
  spellbook_entry *sbe;
  int counter = 0;
  
  sb = create_spellbook(0);
  CuAssertPtrNotNull(tc, sb);
  CuAssertPtrEquals(tc, 0, sb->name);

  sb = create_spellbook("spells");
  CuAssertPtrNotNull(tc, sb);
  CuAssertStrEquals(tc, "spells", sb->name);

  sp = create_spell("testspell", 0);
  spellbook_add(sb, sp, 1);
  CuAssertPtrNotNull(tc, sb->spells);

  sbe = spellbook_get(sb, sp);
  CuAssertPtrNotNull(tc, sbe);
  CuAssertIntEquals(tc, 1, sbe->level);
  CuAssertPtrEquals(tc, sp, sbe->sp);

  spellbook_foreach(sb, count_spell_cb, &counter);
  CuAssertIntEquals(tc, 1, counter);

#ifdef TODO
  /* try adding the same spell twice. that should fail */
  spellbook_add(sb, sp, 1);
  spellbook_foreach(sb, count_spell_cb, &counter);
  CuAssertIntEquals(tc, 1, counter);
#endif
  spellbook_clear(sb);
  free(sb);
}

CuSuite *get_spellbook_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_named_spellbooks);
  return suite;
}
