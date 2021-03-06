#include "ScriptMgr.h"
#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Object.h"
#include "DataMap.h"

/*
Coded by Talamortis - For Azerothcore
Thanks to Rochet for the assistance
*/

class PlayerXpRate : public DataMap::Base
{
public:
    PlayerXpRate() {}
    PlayerXpRate(uint32 XPRate) : XPRate(XPRate) {}
    uint32 XPRate = 1;
};

uint32 MaxRate;

class Individual_XP : public PlayerScript
{
public:
    Individual_XP() : PlayerScript("Individual_XP") { }

    void OnLogin(Player* p) override
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT `XPRate` FROM `individualxp` WHERE `CharacterGUID` = %u", p->GetGUIDLow());
        if (result)
        {
            Field* fields = result->Fetch();
            p->CustomData.Set("Individual_XP", new PlayerXpRate(fields[0].GetUInt32()));
        }
        ChatHandler(p->GetSession()).SendSysMessage("This server is running the |cff4CFF00Individual XP |rmodule. Use .SetXP <amount> to change.");
    }

    void OnLogout(Player* p) override
    {
        if (PlayerXpRate* data = p->CustomData.Get<PlayerXpRate>("Individual_XP"))
        {
            uint32 rate = data->XPRate;
            if (rate <= 1)
                CharacterDatabase.DirectPExecute("DELETE FROM `individualxp` WHERE `CharacterGUID` = %u", p->GetGUIDLow());
            else
                CharacterDatabase.DirectPExecute("REPLACE INTO `individualxp` (`CharacterGUID`, `XPRate`) VALUES (%u, %u);", p->GetGUIDLow(), rate);
        }
    }

    void OnGiveXP(Player* p, uint32& amount, Unit* victim) override
    {
        if (PlayerXpRate* data = p->CustomData.Get<PlayerXpRate>("Individual_XP"))
            amount *= data->XPRate;
    }
};

class Individual_XP_command : public CommandScript
{
public:
    Individual_XP_command() : CommandScript("Individual_XP_command") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> IndividualXPTable =
        {
            { "SetXP", SEC_PLAYER, false, &HandleIndividualXPCommand, "" }
        };
        return IndividualXPTable;
    }

    static bool HandleIndividualXPCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* me = handler->GetSession()->GetPlayer();
        if (!me)
            return false;

        uint32 rate = (uint32)atol(args);
        if (rate == 0 || rate > MaxRate)
            return false;

        me->CustomData.GetDefault<PlayerXpRate>("Individual_XP")->XPRate = rate;
        me->GetSession()->SendAreaTriggerMessage("You have updated your XP rate to %u", rate);
        return true;
    }
};

class Individual_XP_conf : public WorldScript
{
public:
    Individual_XP_conf() : WorldScript("Individual_XP_conf_conf") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            std::string conf_path = _CONF_DIR;
            std::string cfg_file = conf_path + "/Individual-XP.conf";

#ifdef WIN32
            cfg_file = "Individual-XP.conf";
#endif

            std::string cfg_def_file = cfg_file + ".dist";
            sConfigMgr->LoadMore(cfg_def_file.c_str());
            sConfigMgr->LoadMore(cfg_file.c_str());
            MaxRate = sConfigMgr->GetIntDefault("MaxXPRate", 10);
        }
    }
};

void AddIndividual_XPScripts()
{
    new Individual_XP();
    new Individual_XP_conf();
    new Individual_XP_command();
}
