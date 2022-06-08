#include "ScriptMgr.h"
#include "black_temple.h"
#include "GameObjectAI.h"
#include "GridNotifiers.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellScript.h"

enum Yells
{
    SAY_AGGRO                          = 0,
    SAY_NEEDLE                         = 1,
    SAY_SLAY                           = 2,
    SAY_SPECIAL                        = 3,
    SAY_ENRAGE                         = 4,
    SAY_DEATH                          = 5,
};

enum Spells
{
    SPELL_NEEDLE_SPINE                 = 39992,
    SPELL_NEEDLE_SPINE_DAMAGE          = 39835,
    SPELL_TIDAL_BURST                  = 39878,
    SPELL_TIDAL_SHIELD                 = 39872,
    SPELL_IMPALING_SPINE               = 39837,
    SPELL_SUMMON_IMPALING_SPINE        = 39929,
    SPELL_CREATE_NAJENTUS_SPINE        = 39956,
    SPELL_BERSERK                      = 26662,
};

enum Events
{
    EVENT_SPELL_BERSERK                = 1,
    EVENT_YELL                         = 2,
    EVENT_SPELL_NEEDLE                 = 3,
    EVENT_SPELL_SPINE                  = 4,
    EVENT_SPELL_SHIELD                 = 5,
    EVENT_KILL_SPEAK                   = 6,
};

class boss_najentus : public CreatureScript
{
public:
    boss_najentus() : CreatureScript("boss_najentus") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetBlackTempleAI<boss_najentusAI>(creature);
    }

    struct boss_najentusAI : public BossAI
    {
        boss_najentusAI(Creature* creature) : BossAI(creature, DATA_HIGH_WARLORD_NAJENTUS) { }

        void Reset()
        {
            BossAI::Reset();
            SpineTargetGUID.Clear();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && events.GetNextEventTime(EVENT_KILL_SPEAK) == 0)
            {
                Talk(SAY_SLAY);
                events.ScheduleEvent(EVENT_KILL_SPEAK, 5000);
            }
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SPELL_BERSERK, 480000);
            events.ScheduleEvent(EVENT_YELL, urand(25000, 100000));
            events.RescheduleEvent(EVENT_SPELL_NEEDLE, 10000);
            events.RescheduleEvent(EVENT_SPELL_SPINE, 20001);
            events.RescheduleEvent(EVENT_SPELL_SHIELD, 60000);
        }

        bool RemoveImpalingSpine()
        {
            if (!SpineTargetGUID)
                return false;

            Unit* target = ObjectAccessor::GetUnit(*me, SpineTargetGUID);
            if (target && target->HasAura(SPELL_IMPALING_SPINE))
                target->RemoveAurasDueToSpell(SPELL_IMPALING_SPINE);
            SpineTargetGUID.Clear();
            return true;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_SHIELD:
                    me->CastSpell(me, SPELL_TIDAL_SHIELD, false);
                    events.DelayEvents(10000);
                    events.ScheduleEvent(EVENT_SPELL_SHIELD, 60000);
                    break;
                case EVENT_SPELL_BERSERK:
                    Talk(SAY_ENRAGE);
                    me->CastSpell(me, SPELL_BERSERK, true);
                    break;
                case EVENT_SPELL_NEEDLE:
                    me->CastSpell(me, SPELL_NEEDLE_SPINE, SPELLVALUE_MAX_TARGETS);
                    events.ScheduleEvent(EVENT_SPELL_NEEDLE, 15000);
                    break;
                case EVENT_SPELL_SPINE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200.0f, true))
                    {
                        me->CastSpell(target, SPELL_IMPALING_SPINE, false);
                        target->CastSpell(target, SPELL_SUMMON_IMPALING_SPINE, true);
                        SpineTargetGUID = target->GetGUID();
                        Talk(SAY_NEEDLE);
                    }
                    events.ScheduleEvent(EVENT_SPELL_SPINE, 20000);
                    return;
                case EVENT_YELL:
                    Talk(SAY_SPECIAL);
                    events.ScheduleEvent(EVENT_YELL, urand(25000, 100000));
                    break;
            }

            DoMeleeAttackIfReady();
        }
    private:
        ObjectGuid SpineTargetGUID;
    };
};

class spell_najentus_needle_spine : public SpellScriptLoader
{
    public:
        spell_najentus_needle_spine() : SpellScriptLoader("spell_najentus_needle_spine") { }

        class spell_najentus_needle_spine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_najentus_needle_spine_SpellScript);

            void HandleDummy(SpellEffIndex effIndex, int32& damage)
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, SPELL_NEEDLE_SPINE_DAMAGE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_najentus_needle_spine_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_najentus_needle_spine_SpellScript();
        }
};

class spell_najentus_hurl_spine : public SpellScriptLoader
{
    public:
        spell_najentus_hurl_spine() : SpellScriptLoader("spell_najentus_hurl_spine") { }

        class spell_najentus_hurl_spine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_najentus_hurl_spine_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/, int32& damage)
            {
                Unit* target = GetHitUnit();
                if (target && target->HasAura(SPELL_TIDAL_SHIELD))
                {
                    target->RemoveAurasDueToSpell(SPELL_TIDAL_SHIELD);
                    target->CastSpell(target, SPELL_TIDAL_BURST, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_najentus_hurl_spine_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_najentus_hurl_spine_SpellScript();
        }
};

class go_najentus_spine : public GameObjectScript
{
public:
    go_najentus_spine() : GameObjectScript("go_najentus_spine") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = go->GetInstanceScript())
            if (Creature* Najentus = ObjectAccessor::GetCreature(*go, instance->GetGuidData(DATA_HIGH_WARLORD_NAJENTUS)))
                if (CAST_AI(boss_najentus::boss_najentusAI, Najentus->AI())->RemoveImpalingSpine())
                {
                    player->CastSpell(player, SPELL_CREATE_NAJENTUS_SPINE, true);
                    go->Delete();
                }
        return true;
    }

};

void AddSC_boss_najentus()
{
    new boss_najentus();
    new spell_najentus_needle_spine();
    new spell_najentus_hurl_spine();
	new go_najentus_spine();
}
