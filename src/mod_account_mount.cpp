/*
* Account share mounts module for AzerothCore ~2.0.
* copyright (c) since 2018 Dmitry Brusensky <brussens@nativeweb.ru>
* https://github.com/brussens/ac-account-mounts-module
*/
#include "Player.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"


class AccountMounts : public PlayerScript
{
public:
    AccountMounts() : PlayerScript("AccountMounts") { }

    void OnLogin(Player* player)
    {
       if (sConfigMgr->GetOption<bool>("Account.Mounts.Enable", true))
       {
            QueryResult guids = CharacterDatabase.Query(
                "SELECT `guid` FROM `characters` WHERE `account` = {} AND `guid` <> {}",
                player->GetSession()->GetAccountId(), player->GetGUID().GetCounter()
            );

            if (!guids)
                return;

            QueryResult spells = CharacterDatabase.Query("SELECT DISTINCT `spell` FROM `character_spell` WHERE `guid` IN({})", implodeGuids(guids).c_str());

            if (!spells)
                return;

            do {
                const SpellEntry* spell = sSpellStore.LookupEntry(spells->Fetch()[0].Get<uint32>());

                if (!player->HasSpell(spell->Id) && isSpellCompatible(spell, player) && hasRidingSkill(player))
                    player->learnSpell(spell->Id);

            } while (spells->NextRow());
       }
	}
private:
    /*
    * Glue character guids in line for spell query.
    */
    std::string implodeGuids(QueryResult &queryResult)
    {
        uint8 i = 0;
        std::string condition;

        do {
            if (i)
                condition += ",";

            condition += std::to_string(queryResult->Fetch()[0].Get<uint32>());
            ++i;
        } while (queryResult->NextRow());

        return condition;
    }

    /*
    * Check all compatibility for spell.
    */
    bool isSpellCompatible(const SpellEntry* spell, Player* player)
    {
        return isMount(spell) && isRaceCompatible(spell, player) && isClassCompatible(spell, player);
    }

    /*
    * Check spell on mount type.
    */
    bool isMount(const SpellEntry* spell)
    {
        return spell->Effect[0] == SPELL_EFFECT_APPLY_AURA && spell->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED;
    }

    /*
    * Check spell on race compatibility.
    */
    bool isRaceCompatible(const SpellEntry* spell, Player* player)
    {
        if (sConfigMgr->GetOption<bool>("Account.Mounts.StrictRace", true))
        {
            if (spell->AttributesEx7 & SPELL_ATTR7_ALLIANCE_SPECIFIC_SPELL)
            {
                return Player::TeamIdForRace(player->getRace()) == TEAM_ALLIANCE;
            }
            else if (spell->AttributesEx7 & SPELL_ATTR7_HORDE_SPECIFIC_SPELL)
            {
                return Player::TeamIdForRace(player->getRace()) == TEAM_HORDE;
            }
        }
        return true;
    }

    /*
    * Check spell on class compatibility.
    */
    bool isClassCompatible(const SpellEntry* spell, Player* player)
    {
        if (sConfigMgr->GetOption<bool>("Account.Mounts.StrictClass", true))
        {
            switch (spell->SpellFamilyName) {
                case SPELLFAMILY_PALADIN: return isValidClass(CLASS_PALADIN, player);
                case SPELLFAMILY_DEATHKNIGHT: return isValidClass(CLASS_DEATH_KNIGHT, player);
                case SPELLFAMILY_WARLOCK: return isValidClass(CLASS_WARLOCK, player);
                default: return true;
            }
        }
        return true;
    }

    /*
    * Check if player has Riding skill
    */
    bool hasRidingSkill(Player* player)
    {
        return player->HasSkill(SKILL_RIDING);
    }

    bool isValidClass(uint8 const &ClassID, Player* player)
    {
        return player->getClass() == ClassID;
    }
};

void AddAccountMountsScripts()
{
    new AccountMounts();
}
