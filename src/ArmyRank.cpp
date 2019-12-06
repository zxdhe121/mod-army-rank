// army rank engine

#include "ArmyRank.h"
#include "chat.h"
#include "player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Log.h"
#include <unordered_map>

enum {
    LANG_ARMY_RANK_BUY_ITEM_ERROR = 60001,
    LANG_ARMY_RANK_INFO = 60002,
    LANG_ARMY_RANK_USE_ITEM_ERROR = 60003,
    LANG_ARMY_RANK_LEVEL_UP = 60004,
    ARMY_RANK_NAME_INDEX = 60100,

    LANG_ARMY_RANK_VENDOR_OPTION_S1 = 60011,
    LANG_ARMY_RANK_VENDOR_OPTION_S2 = 60012,
    LANG_ARMY_RANK_VENDOR_OPTION_S3 = 60013,
    LANG_ARMY_RANK_VENDOR_OPTION_S4 = 60014,
    LANG_ARMY_RANK_VENDOR_OPTION_VETERAN = 60015,
    LANG_ARMY_RANK_VENDOR_OPTION_VINDICATOR = 60016,
    LANG_ARMY_RANK_VENDOR_OPTION_GUARDIAN = 60017,
    LANG_ARMY_RANK_VENDOR_OPTION_CLOSE = 60018,
    LANG_ARMY_RANK_VENDOR_OPTION_RETURN = 60019,

    ARMY_RANK_VENDOR_REAL_ENTRY_S1 = 61091,
    ARMY_RANK_VENDOR_REAL_ENTRY_S2 = 61092,
    ARMY_RANK_VENDOR_REAL_ENTRY_S3 = 61093,
    ARMY_RANK_VENDOR_REAL_ENTRY_S4 = 61094,
    ARMY_RANK_VENDOR_REAL_ENTRY_VETERAN = 61080,
    ARMY_RANK_VENDOR_REAL_ENTRY_VINDICATOR = 61081,
    ARMY_RANK_VENDOR_REAL_ENTRY_GUARDIAN = 61082,


    LANG_ARMY_HEROIC_VENDOR_OPTION_S1 = 60031,
    LANG_ARMY_HEROIC_VENDOR_OPTION_S2 = 60032,
    LANG_ARMY_HEROIC_VENDOR_OPTION_S3 = 60033,
    LANG_ARMY_HEROIC_VENDOR_OPTION_S4 = 60034,
    LANG_ARMY_HEROIC_VENDOR_OPTION_VETERAN = 60035,
    LANG_ARMY_HEROIC_VENDOR_OPTION_VINDICATOR = 60036,
    LANG_ARMY_HEROIC_VENDOR_OPTION_GUARDIAN = 60037,
    LANG_ARMY_HEROIC_VENDOR_OPTION_CLOSE = 60038,
    LANG_ARMY_HEROIC_VENDOR_OPTION_RETURN = 60039,

    ARMY_HEROIC_VENDOR_REAL_ENTRY_S1 = 61095,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_S2 = 61096,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_S3 = 61097,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_S4 = 61098,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_VETERAN = 61076,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_VINDICATOR = 61077,
    ARMY_HEROIC_VENDOR_REAL_ENTRY_GUARDIAN = 61078,


    LANG_ARMY_VENDOR_OTHERS_OPTION_RANKUP = 60201,
    LANG_ARMY_VENDOR_OTHERS_OPTION_SHIRT = 60202,
    LANG_ARMY_VENDOR_OTHERS_OPTION_CONVERT = 60203,
    LANG_ARMY_VENDOR_OTHERS_OPTION_CLOSE = 600204,

    ARMY_VENDOR_OTHERS_REAL_ENTRY__RANKUP = 61089,
    ARMY_VENDOR_OTHERS_REAL_ENTRY_SHIRT = 61090,
    ARMY_VENDOR_OTHERS_REAL_ENTRY_CONVERT = 61079,
};

ArmyRank::ArmyRank() { }

ArmyRank::~ArmyRank()
{
    for (std::unordered_map<uint64, uint8>::iterator itr = PlayerRank.begin(); itr != PlayerRank.end(); ++itr)
        delete& itr->second;
    for (std::unordered_map<uint32, uint8>::iterator itr = ItemRank.begin(); itr != ItemRank.end(); ++itr)
        delete& itr->second;

    PlayerRank.clear();
    ItemRank.clear();
}

ArmyRank* ArmyRank::instance()
{
    static ArmyRank instance;
    return &instance;
}

class ArmyRank_Load_Config : public WorldScript
{
public: ArmyRank_Load_Config() : WorldScript("ArmyRank_Load_Config") { };

        void OnAfterConfigLoad(bool /*reload*/)
        {
            sLog->outString(" ArmyRank Engine load config");

            QueryResult itemQuery = WorldDatabase.PQuery("SELECT entry, army_rank FROM item_template;");
            if (itemQuery)
            {
                do
                {
                    Field* fields = itemQuery->Fetch();
                    uint32 item_id = fields[0].GetUInt32();
                    uint8 rank = fields[1].GetUInt8();

                    // Save the DB values to the MyData object
                    sArmyRank->SetItemRank(item_id, rank);

                } while (itemQuery->NextRow());
            }
            sLog->outString(" ArmyRank Engine config loaded");
        }
};

void ArmyRank::SetPlayerRank(uint64 player_guid, uint8 rank, bool update_DB)
{
    sArmyRank->PlayerRank[player_guid] = rank;
    if (update_DB) {
        CharacterDatabase.PExecute("UPDATE characters SET army_rank = %u WHERE guid = %u ;", sArmyRank->PlayerRank[player_guid], player_guid);
    }

}

class ArmyRank_Player_Script : public PlayerScript
{
public: ArmyRank_Player_Script() : PlayerScript("ArmyRank_Player_Script") { };

        void OnLoadFromDB(Player* player)
        {
            uint64 playerGUID = player->GetGUID();
            QueryResult PlayerQuery = CharacterDatabase.PQuery("SELECT army_rank FROM characters WHERE guid = %u ;", playerGUID);
            Field* fields = PlayerQuery->Fetch();
            uint8 rank = fields[0].GetUInt8();
            // Save the DB values to the MyData object
            sArmyRank->SetPlayerRank(playerGUID, rank, false);

            //bonus talent points
            if (rank && rank > 0) {
                player->RewardOtherBonusTalentPoints(rank);
            }
        }

        bool OnBeforeBuyItemFromVendor(Player* player, uint64 /*vendorguid*/, uint32 /*vendorslot*/, uint32& item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/) {
            uint8 playerRank = sArmyRank->GetPlayerRank(player->GetGUID());
            uint8 itemRank = sArmyRank->GetItemRank(item);
            if (itemRank > playerRank) {
                player->GetSession()->SendAreaTriggerMessage(player->GetSession()->GetTrinityString(LANG_ARMY_RANK_BUY_ITEM_ERROR), playerRank, itemRank);
                return false;
            }
            return true;
        }

};

class ArmyRank_Info_Show_Script : public ItemScript
{
public: ArmyRank_Info_Show_Script() : ItemScript("ArmyRank_Info_Show_Script") { };

        bool OnUse(Player* player, Item* item, SpellCastTargets const& targets)
        {
            uint64 playerGUID = player->GetGUID();
            uint8 rank = sArmyRank->GetPlayerRank(playerGUID);

            ChatHandler(player->GetSession()).PSendSysMessage("**********************************");
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARMY_RANK_INFO, (player->GetSession()->GetTrinityString(ARMY_RANK_NAME_INDEX + rank)), rank);
            ChatHandler(player->GetSession()).PSendSysMessage("**********************************");
            return true;
        }
};

class ArmyRank_Up_Script : public ItemScript
{
public: ArmyRank_Up_Script() : ItemScript("ArmyRank_Up_Script") { };

        bool OnUse(Player* player, Item* item, SpellCastTargets const& targets)
        {
            uint64 playerGUID = player->GetGUID();
            uint8 playerRank = sArmyRank->GetPlayerRank(playerGUID);
            uint8 itemRank = sArmyRank->GetItemRank(item->GetEntry());
            if (itemRank != playerRank) {
                player->GetSession()->SendAreaTriggerMessage(player->GetSession()->GetTrinityString(LANG_ARMY_RANK_USE_ITEM_ERROR), playerRank, itemRank);
                return false;
            }
            sArmyRank->SetPlayerRank(playerGUID, playerRank + 1, true);

            player->RewardOtherBonusTalentPoints(1);
            player->InitTalentForLevel();
            player->DestroyItemCount(item->GetEntry(), 1, true);
            player->CastSpell(player, 31726);//视觉效果
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARMY_RANK_LEVEL_UP, (player->GetSession()->GetTrinityString(ARMY_RANK_NAME_INDEX + playerRank + 1)), (playerRank + 1));
            return true;
        }
};


class ArmyRank_season_vendor_Script : public CreatureScript
{
public:
    ArmyRank_season_vendor_Script() : CreatureScript("ArmyRank_season_vendor_Script") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        sendMainMenu(player, creature);
        return true;
    }

    void sendMainMenu(Player* player, Creature* creature)
    {
        uint8 arenaSeason = sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID);
        if (arenaSeason >= 1) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_S1), GOSSIP_SENDER_MAIN, 1);
        }
        if (arenaSeason >= 2) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_S2), GOSSIP_SENDER_MAIN, 2);
        }
        if (arenaSeason >= 3) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_S3), GOSSIP_SENDER_MAIN, 3);
        }
        if (arenaSeason >= 4) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_S4), GOSSIP_SENDER_MAIN, 4);
        }
        if (arenaSeason >= 1) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_VETERAN), GOSSIP_SENDER_MAIN, 5);
        }
        if (arenaSeason >= 3) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_VINDICATOR), GOSSIP_SENDER_MAIN, 6);
        }
        if (arenaSeason >= 4) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_GUARDIAN), GOSSIP_SENDER_MAIN, 7);
        }
        player->ADD_GOSSIP_ITEM(0, player->GetSession()->GetTrinityString(LANG_ARMY_RANK_VENDOR_OPTION_CLOSE), GOSSIP_SENDER_MAIN, 99);
        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
        case 1:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_S1);
            break;

        case 2:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_S2);
            break;

        case 3:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_S3);
            break;

        case 4:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_S4);
            break;

        case 5:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_VETERAN);
            break;

        case 6:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_VINDICATOR);
            break;

        case 7:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_RANK_VENDOR_REAL_ENTRY_GUARDIAN);
            break;

        default:
            break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

class ArmyRank_season_vendor_heroic_Script : public CreatureScript
{
public:
    ArmyRank_season_vendor_heroic_Script() : CreatureScript("ArmyRank_season_vendor_heroic_Script") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        sendMainMenu(player, creature);
        return true;
    }

    void sendMainMenu(Player* player, Creature* creature)
    {
        uint8 arenaSeason = sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID);
        if (arenaSeason >= 1) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_S1), GOSSIP_SENDER_MAIN, 1);
        }
        if (arenaSeason >= 2) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_S2), GOSSIP_SENDER_MAIN, 2);
        }
        if (arenaSeason >= 3) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_S3), GOSSIP_SENDER_MAIN, 3);
        }
        if (arenaSeason >= 4) {
            player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_S4), GOSSIP_SENDER_MAIN, 4);
        }
        //player->ADD_GOSSIP_ITEM(10, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_VETERAN), GOSSIP_SENDER_MAIN, 5);
        //player->ADD_GOSSIP_ITEM(10, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_VINDICATOR), GOSSIP_SENDER_MAIN, 6);
        //player->ADD_GOSSIP_ITEM(10, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_GUARDIAN), GOSSIP_SENDER_MAIN, 7);
        player->ADD_GOSSIP_ITEM(0, player->GetSession()->GetTrinityString(LANG_ARMY_HEROIC_VENDOR_OPTION_CLOSE), GOSSIP_SENDER_MAIN, 99);
        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
        case 1:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_S1);
            break;

        case 2:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_S2);
            break;

        case 3:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_S3);
            break;

        case 4:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_S4);
            break;


            /*
            case 5:
                player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_VETERAN);
                break;

            case 6:
                player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_VINDICATOR);
                break;

            case 7:
                player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_HEROIC_VENDOR_REAL_ENTRY_GUARDIAN);
                break;
             */

        default:
            break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

class ArmyRank_vendor_others_Script : public CreatureScript
{
public:
    ArmyRank_vendor_others_Script() : CreatureScript("ArmyRank_vendor_others_Script") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        sendMainMenu(player, creature);
        return true;
    }

    void sendMainMenu(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_VENDOR_OTHERS_OPTION_RANKUP), GOSSIP_SENDER_MAIN, 1);
        player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_VENDOR_OTHERS_OPTION_SHIRT), GOSSIP_SENDER_MAIN, 2);
        player->ADD_GOSSIP_ITEM(1, player->GetSession()->GetTrinityString(LANG_ARMY_VENDOR_OTHERS_OPTION_CONVERT), GOSSIP_SENDER_MAIN, 3);
        player->ADD_GOSSIP_ITEM(0, player->GetSession()->GetTrinityString(LANG_ARMY_VENDOR_OTHERS_OPTION_CLOSE), GOSSIP_SENDER_MAIN, 99);
        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
        case 1:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_VENDOR_OTHERS_REAL_ENTRY__RANKUP);
            break;

        case 2:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_VENDOR_OTHERS_REAL_ENTRY_SHIRT);
            break;

        case 3:
            player->GetSession()->SendListInventoryCustom(creature->GetGUID(), ARMY_VENDOR_OTHERS_REAL_ENTRY_CONVERT);
            break;

        default:
            break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};


void AddSC_ArmyRank()
{
    new ArmyRank_Load_Config;
    new ArmyRank_Player_Script;
    new ArmyRank_Info_Show_Script;
    new ArmyRank_Up_Script;
    new ArmyRank_season_vendor_Script;
    new ArmyRank_season_vendor_heroic_Script;
    new ArmyRank_vendor_others_Script;
}
