#include <platform.h>
#include "kernel/types.h"
#include "kernel/config.h"
#include "keyword.h"
#include "util/language.h"
#include "tests.h"

#include <critbit.h>
#include <CuTest.h>

static void test_init_keywords(CuTest *tc) {
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, "keyword::move", "MOVE");
    init_keywords(lang);
    CuAssertIntEquals(tc, K_MOVE, get_keyword("move", lang));
    test_cleanup();
}

static void test_init_keyword(CuTest *tc) {
    struct locale *lang;
    test_cleanup();

    lang = get_or_create_locale("de");
    init_keyword(lang, K_MOVE, "NACH");
    init_keyword(lang, K_STUDY, "LERNEN");
    init_keyword(lang, K_DESTROY, "ZERSTOEREN");
    CuAssertIntEquals(tc, K_MOVE, get_keyword("nach", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERN", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("LERNEN", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("lerne", lang));
    CuAssertIntEquals(tc, K_DESTROY, get_keyword("zerst\xC3\xB6ren", lang));
    CuAssertIntEquals(tc, NOKEYWORD, get_keyword("potato", lang));
    test_cleanup();
}

static void test_findkeyword(CuTest *tc) {
    test_cleanup();
    CuAssertIntEquals(tc, K_MOVE, findkeyword("move"));
    CuAssertIntEquals(tc, K_STUDY, findkeyword("study"));
    CuAssertIntEquals(tc, NOKEYWORD, findkeyword(""));
    CuAssertIntEquals(tc, NOKEYWORD, findkeyword("potato"));
}

static void test_get_keyword_default(CuTest *tc) {
    struct locale *lang;
    test_cleanup();
    lang = get_or_create_locale("en");
    CuAssertIntEquals(tc, NOKEYWORD, get_keyword("potato", lang));
    CuAssertIntEquals(tc, K_MOVE, get_keyword("move", lang));
    CuAssertIntEquals(tc, K_STUDY, get_keyword("study", lang));
}

static void test_get_shortest_match(CuTest *tc) {
    struct locale *lang;
    critbit_tree ** cb;

    test_cleanup();
    lang = get_or_create_locale("en");

    cb = (critbit_tree **)get_translations(lang, UT_KEYWORDS);
    /* note that the english order is FIGHT, not COMBAT, so this is a poor example */
    add_translation(cb, "COMBAT", K_STATUS);
    add_translation(cb, "COMBATSPELL", K_COMBATSPELL);

    CuAssertIntEquals(tc, NOKEYWORD, get_keyword("", lang));
    CuAssertIntEquals(tc, K_STATUS, get_keyword("COM", lang));
    CuAssertIntEquals(tc, K_STATUS, get_keyword("COMBAT", lang));
    CuAssertIntEquals(tc, K_COMBATSPELL, get_keyword("COMBATS", lang));
}

#define SUITE_DISABLE_TEST(suite, test) (void)test

CuSuite *get_keyword_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_init_keyword);
  SUITE_ADD_TEST(suite, test_init_keywords);
  SUITE_ADD_TEST(suite, test_findkeyword);
  SUITE_ADD_TEST(suite, test_get_shortest_match);
  SUITE_DISABLE_TEST(suite, test_get_keyword_default);
  return suite;
}

