/*
* Account share mounts module for AzerothCore ~2.0.
* copyright (c) since 2018 Dmitry Brusensky <brussens@nativeweb.ru>
* https://github.com/brussens/ac-account-mounts-module
*/
#include "Config.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include <set>         // Required for std::set             -- might be redundant
#include <sstream>     // Required for std::istringstream   -- might be redundant
#include <string>      // Required for std::string          -- might be redundant

class AccountMounts : public PlayerScript
{
    static const bool limitrace = true; // This set to true will only learn mounts from chars on the same team, do what you want.
    std::set<uint32> excludedSpellIds; // Set to hold the Spell IDs to be excluded

public:
    AccountMounts() : PlayerScript("AccountMounts")
    {
        // Retrieve the string of excluded Spell IDs from the config file
        std::string excludedSpellsStr = sConfigMgr->GetOption<std::string>("Account.Mounts.ExcludedSpellIDs", "");
        // Proceed only if the configuration is not "0" or empty, indicating exclusions are specified
        if (excludedSpellsStr != "0" && !excludedSpellsStr.empty())
        {
            std::istringstream spellStream(excludedSpellsStr);
            std::string spellIdStr;
            while (std::getline(spellStream, spellIdStr, ',')) {
                uint32 spellId = static_cast<uint32>(std::stoul(spellIdStr));
                if (spellId != 0) { // Ensure the spell ID is not 0, as 0 is used to indicate no exclusions
                    excludedSpellIds.insert(spellId); // Add the Spell ID to the set of exclusions
                }
            }
        }
    }
    
    void OnLogin(Player* pPlayer)
    {
       if (sConfigMgr->GetOption<bool>("Account.Mounts.Enable", true))
       {
            QueryResult guids = CharacterDatabase.Query(
                "SELECT `guid` FROM `characters` WHERE `account` = {} AND `guid` <> {}",
                player->GetSession()->GetAccountId(), player->GetGUID().GetCounter()
            );

            std::vector<uint32> Guids;
            uint32 playerAccountID = pPlayer->GetSession()->GetAccountId();
            QueryResult result1 = CharacterDatabase.Query("SELECT `guid`, `race` FROM `characters` WHERE `account`={};", playerAccountID);

            if (!result1)
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
                Field* fields = result1->Fetch();
                uint32 race = fields[1].Get<uint8>();

                if ((Player::TeamIdForRace(race) == Player::TeamIdForRace(pPlayer->getRace())) || !limitrace)
                    Guids.push_back(fields[0].Get<uint32>());

            } while (result1->NextRow());

            std::vector<uint32> Spells;

            for (auto& i : Guids)
            {
                QueryResult result2 = CharacterDatabase.Query("SELECT `spell` FROM `character_spell` WHERE `guid`={};", i);
                if (!result2)
                    continue;

                do
                {
                    Spells.push_back(result2->Fetch()[0].Get<uint32>());
                } while (result2->NextRow());
            }
            else if (spell->AttributesEx7 & SPELL_ATTR7_HORDE_SPECIFIC_SPELL)
            {
                // Check if the spell is in the excluded list before learning it
                if (excludedSpellIds.find(i) == excludedSpellIds.end())
                {
                    auto sSpell = sSpellStore.LookupEntry(i);
                    if (sSpell->Effect[0] == SPELL_EFFECT_APPLY_AURA && sSpell->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED)
                        pPlayer->learnSpell(sSpell->Id);
                }
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
