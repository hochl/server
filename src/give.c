/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2014   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

 */
#include <platform.h>
#include <kernel/config.h>
#include "give.h"

#include "economy.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/orcification.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

/* Wieviel Fremde eine Partei pro Woche aufnehmen kangiven */
#define MAXNEWBIES								5
#define RESERVE_DONATIONS       /* shall we reserve objects given to us by other factions? */
#define RESERVE_GIVE            /* reserve anything that's given from one unit to another? */

static int GiveRestriction(void)
{
    static int value = -1;
    if (value < 0) {
        const char *str = get_param(global.parameters, "GiveRestriction");
        value = str ? atoi(str) : 0;
    }
    return value;
}

static void
add_give(unit * u, unit * u2, int given, int received,
const resource_type * rtype, struct order *ord, int error)
{
    if (error) {
        cmistake(u, ord, error, MSG_COMMERCE);
    }
    else if (u2 == NULL) {
        ADDMSG(&u->faction->msgs,
            msg_message("give_peasants", "unit resource amount", u, rtype, given));
    }
    else if (u2->faction != u->faction) {
        message *msg;

        msg =
            msg_message("give", "unit target resource amount", u, u2, rtype, given);
        add_message(&u->faction->msgs, msg);
        msg_release(msg);

        msg =
            msg_message("receive", "unit target resource amount", u, u2, rtype,
            received);
        add_message(&u2->faction->msgs, msg);
        msg_release(msg);
    }
}

static bool limited_give(const item_type * type)
{
    /* trade only money 2:1, if at all */
    return (type->rtype == get_resourcetype(R_SILVER));
}

int give_quota(const unit * src, const unit * dst, const item_type * type,
    int n)
{
    float divisor;

    if (!limited_give(type)) {
        return n;
    }
    if (dst && src && src->faction != dst->faction) {
        divisor = get_param_flt(global.parameters, "rules.items.give_divisor", 1);
        assert(divisor == 0 || divisor >= 1);
        if (divisor >= 1) {
            /* predictable > correct: */
            int x = (int)(n / divisor);
            return x;
        }
    }
    return n;
}

int
give_item(int want, const item_type * itype, unit * src, unit * dest,
struct order *ord)
{
    short error = 0;
    int n, r;

    assert(itype != NULL);
    n = get_pooled(src, item2resource(itype), GET_SLACK | GET_POOLED_SLACK, want);
    n = _min(want, n);
    r = n;
    if (dest && src->faction != dest->faction
        && src->faction->age < GiveRestriction()) {
        if (ord != NULL) {
            ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "giverestriction",
                "turns", GiveRestriction()));
        }
        return -1;
    }
    else if (n == 0) {
        int reserve = get_reservation(src, itype->rtype);
        if (reserve) {
            msg_feedback(src, ord, "nogive_reserved", "resource reservation",
                itype->rtype, reserve);
            return -1;
        }
        error = 36;
    }
    else if (itype->flags & ITF_CURSED) {
        error = 25;
    }
    else if (itype->give == NULL || itype->give(src, dest, itype, n, ord) != 0) {
        int use = use_pooled(src, item2resource(itype), GET_SLACK, n);
        if (use < n)
            use +=
            use_pooled(src, item2resource(itype), GET_POOLED_SLACK,
            n - use);
        if (dest) {
            r = give_quota(src, dest, itype, n);
            i_change(&dest->items, itype, r);
#ifdef RESERVE_GIVE
#ifdef RESERVE_DONATIONS
            change_reservation(dest, item2resource(itype), r);
#else
            if (src->faction == dest->faction) {
                change_reservation(dest, item2resource(itype), r);
            }
#endif
#endif
#if MUSEUM_MODULE && defined(TODO)
            /* TODO: use a trigger for the museum warden! */
            if (a_find(dest->attribs, &at_warden)) {
                warden_add_give(src, dest, itype, r);
            }
#endif
            handle_event(dest->attribs, "receive", src);
        }
        handle_event(src->attribs, "give", dest);
    }
    add_give(src, dest, n, r, item2resource(itype), ord, error);
    if (error)
        return -1;
    return 0;
}

void give_men(int n, unit * u, unit * u2, struct order *ord)
{
    ship *sh;
    int k = 0;
    int error = 0;

    if (u2 && u->faction != u2->faction && u->faction->age < GiveRestriction()) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "giverestriction",
            "turns", GiveRestriction()));
        return;
    }
    else if (u == u2) {
        error = 10;
    }
    else if (!u2 && u_race(u) == get_race(RC_SNOTLING)) {
        /* snotlings may not be given to the peasants. */
        error = 307;
    }
    else if (u2 && u2->number && (fval(u, UFL_HERO) != fval(u2, UFL_HERO))) {
        /* heroes may not be given to non-heroes and vice versa */
        error = 75;
    }
    else if (unit_has_cursed_item(u) || (u2 && unit_has_cursed_item(u2))) {
        error = 78;
    }
    else if (fval(u, UFL_LOCKED) || is_cursed(u->attribs, C_SLAVE, 0)) {
        error = 74;
    }
    else if (u2 && fval(u, UFL_HUNGER)) {
        /* hungry people cannot be given away */
        error = 73;
    }
    else if (u2 && (fval(u2, UFL_LOCKED) || is_cursed(u2->attribs, C_SLAVE, 0))) {
        error = 75;
    }
    else if (u2 && !ucontact(u2, u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact",
            "target", u2));
        error = -1;
    }
    else if (u2 && (has_skill(u, SK_MAGIC) || has_skill(u2, SK_MAGIC))) {
        /* cannot give units to and from magicians */
        error = 158;
    }
    else if (u2 && (fval(u, UFL_WERE) != fval(u2, UFL_WERE))) {
        /* werewolves can't be given to non-werewolves and vice-versa */
        error = 312;
    }
    else if (u2 && u2->number != 0 && u_race(u2) != u_race(u)) {
        log_debug("faction %s attempts to give %s to %s.\n", itoa36(u->faction->no), u_race(u)->_name, u_race(u2)->_name);
        error = 139;
    }
    else if (u2 != NULL && (get_racename(u2->attribs)
        || get_racename(u->attribs))) {
        error = 139;
    }
    else if (u2 && u2->faction != u->faction && !rule_transfermen()) {
        error = 74;
    }
    else {
        if (n > u->number)
            n = u->number;
        if (u2 && n + u2->number > UNIT_MAXSIZE) {
            n = UNIT_MAXSIZE - u2->number;
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_unit_size",
                "maxsize", UNIT_MAXSIZE));
            assert(n >= 0);
        }
        if (n == 0) {
            error = 96;
        }
        else if (u2 && u->faction != u2->faction) {
            if (u2->faction->newbies + n > MAXNEWBIES) {
                error = 129;
            }
            else if (u_race(u) != u2->faction->race) {
                if (u2->faction->race != get_race(RC_HUMAN)) {
                    error = 120;
                }
                else if (count_migrants(u2->faction) + n >
                    count_maxmigrants(u2->faction)) {
                    error = 128;
                }
                else if (has_limited_skills(u) || has_limited_skills(u2)) {
                    error = 154;
                }
                else if (u2->number != 0) {
                    error = 139;
                }
            }
        }
    }

    if (u2 && (has_skill(u, SK_ALCHEMY) || has_skill(u2, SK_ALCHEMY))) {
        k = count_skill(u2->faction, SK_ALCHEMY);

        /* Falls die Zieleinheit keine Alchemisten sind, werden sie nun
         * welche. */
        if (!has_skill(u2, SK_ALCHEMY) && has_skill(u, SK_ALCHEMY))
            k += u2->number;

        /* Wenn in eine Alchemisteneinheit Personen verschoben werden */
        if (has_skill(u2, SK_ALCHEMY) && !has_skill(u, SK_ALCHEMY))
            k += n;

        /* Wenn Parteigrenzen �berschritten werden */
        if (u2->faction != u->faction)
            k += n;

        /* wird das Alchemistenmaximum ueberschritten ? */

        if (k > skill_limit(u2->faction, SK_ALCHEMY)) {
            error = 156;
        }
    }

    if (error == 0) {
        if (u2 && u2->number == 0) {
            set_racename(&u2->attribs, get_racename(u->attribs));
            u_setrace(u2, u_race(u));
            u2->irace = u->irace;
            if (fval(u, UFL_HERO))
                fset(u2, UFL_HERO);
            else
                freset(u2, UFL_HERO);
        }

        if (u2) {
            /* Einheiten von Schiffen k�nnen nicht NACH in von
             * Nicht-alliierten bewachten Regionen ausf�hren */
            sh = leftship(u);
            if (sh) {
                set_leftship(u2, sh);
            }
            transfermen(u, u2, n);
            if (u->faction != u2->faction) {
                u2->faction->newbies += n;
            }
        }
        else {
            if (getunitpeasants) {
#ifdef ORCIFICATION
                if (u_race(u) == get_race(RC_SNOTLING) && !fval(u->region, RF_ORCIFIED)) {
                    attrib *a = a_find(u->region->attribs, &at_orcification);
                    if (!a)
                        a = a_add(&u->region->attribs, a_new(&at_orcification));
                    a->data.i += n;
                }
#endif
                transfermen(u, NULL, n);
            }
            else {
                error = 159;
            }
        }
    }
    if (error > 0) {
        cmistake(u, ord, error, MSG_COMMERCE);
    }
    else if (!u2) {
        ADDMSG(&u->faction->msgs,
            msg_message("give_person_peasants", "unit amount", u, n));
    }
    else if (u2->faction != u->faction) {
        message *msg = msg_message("give_person", "unit target amount", u, u2, n);
        add_message(&u->faction->msgs, msg);
        add_message(&u2->faction->msgs, msg);
        msg_release(msg);
    }
}

void give_unit(unit * u, unit * u2, order * ord)
{
    region *r = u->region;
    int n = u->number;

    if (!rule_transfermen() && u->faction != u2->faction) {
        cmistake(u, ord, 74, MSG_COMMERCE);
        return;
    }

    if (u && unit_has_cursed_item(u)) {
        cmistake(u, ord, 78, MSG_COMMERCE);
        return;
    }

    if (fval(u, UFL_HERO)) {
        cmistake(u, ord, 75, MSG_COMMERCE);
        return;
    }
    if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)) {
        cmistake(u, ord, 74, MSG_COMMERCE);
        return;
    }

    if (u2 == NULL) {
        if (fval(r->terrain, SEA_REGION)) {
            cmistake(u, ord, 152, MSG_COMMERCE);
        }
        else if (getunitpeasants) {
            unit *u3;

            for (u3 = r->units; u3; u3 = u3->next)
                if (u3->faction == u->faction && u != u3)
                    break;

            if (u3) {
                while (u->items) {
                    item *iold = i_remove(&u->items, u->items);
                    item *inew = *i_find(&u3->items, iold->type);
                    if (inew == NULL)
                        i_add(&u3->items, iold);
                    else {
                        inew->number += iold->number;
                        i_free(iold);
                    }
                }
            }
            give_men(u->number, u, NULL, ord);
            cmistake(u, ord, 153, MSG_COMMERCE);
        }
        else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
                ""));
        }
        return;
    }

    if (!alliedunit(u2, u->faction, HELP_GIVE) && ucontact(u2, u) == 0) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact",
            "target", u2));
        return;
    }
    if (u->number == 0) {
        cmistake(u, ord, 105, MSG_COMMERCE);
        return;
    }
    if (u2->faction->newbies + n > MAXNEWBIES) {
        cmistake(u, ord, 129, MSG_COMMERCE);
        return;
    }
    if (u_race(u) != u2->faction->race) {
        if (u2->faction->race != get_race(RC_HUMAN)) {
            cmistake(u, ord, 120, MSG_COMMERCE);
            return;
        }
        if (count_migrants(u2->faction) + u->number >
            count_maxmigrants(u2->faction)) {
            cmistake(u, ord, 128, MSG_COMMERCE);
            return;
        }
        if (has_limited_skills(u)) {
            cmistake(u, ord, 154, MSG_COMMERCE);
            return;
        }
    }
    if (has_skill(u, SK_MAGIC)) {
        sc_mage *mage;
        if (count_skill(u2->faction, SK_MAGIC) + u->number >
            skill_limit(u2->faction, SK_MAGIC)) {
            cmistake(u, ord, 155, MSG_COMMERCE);
            return;
        }
        mage = get_mage(u);
        if (!mage || u2->faction->magiegebiet != mage->magietyp) {
            cmistake(u, ord, 157, MSG_COMMERCE);
            return;
        }
    }
    if (has_skill(u, SK_ALCHEMY)
        && count_skill(u2->faction, SK_ALCHEMY) + u->number >
        skill_limit(u2->faction, SK_ALCHEMY)) {
        cmistake(u, ord, 156, MSG_COMMERCE);
        return;
    }
    add_give(u, u2, 1, 1, get_resourcetype(R_UNIT), ord, 0);
    u_setfaction(u, u2->faction);
    u2->faction->newbies += n;
}
