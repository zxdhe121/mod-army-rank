// army rank engine

#include "ArmyRank.h"
#include "chat.h"
#include "player.h"
#include "ScriptMgr.h"
#include "Log.h"
#include <unordered_map>

enum {
    LANG_ARMY_RANK_BUY_ITEM_ERROR = 60001,
    LANG_ARMY_RANK_INFO = 60002,
    LANG_ARMY_RANK_USE_ITEM_ERROR = 60003,
    LANG_ARMY_RANK_LEVEL_UP = 60004,
};

ArmyRank::ArmyRank() { }

ArmyRank::~ArmyRank()
{
	for (std::unordered_map<uint64, uint8>::iterator itr = PlayerRank.begin(); itr != PlayerRank.end(); ++itr)
		delete &itr->second;
	for (std::unordered_map<uint32, uint8>::iterator itr = ItemRank.begin(); itr != ItemRank.end(); ++itr)
		delete &itr->second;

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
	if(update_DB){
        CharacterDatabase.PExecute("UPDATE characters SET `army_rank`='%u' WHERE `guid`='%u';", sArmyRank->PlayerRank[player_guid], player_guid);
    }

}

class ArmyRank_Player_Script : public PlayerScript
{
public: ArmyRank_Player_Script() : PlayerScript("ArmyRank_Player_Script") { };

		void OnLogin(Player* player)
		{
		    uint64 playerGUID = player->GetGUID();
			QueryResult PlayerQuery = CharacterDatabase.PQuery("SELECT `army_rank` FROM characters WHERE `guid`='%u';", playerGUID);
            Field* fields = PlayerQuery->Fetch();
            uint8 rank = fields[0].GetUInt8();
            // Save the DB values to the MyData object
            sArmyRank->SetPlayerRank(playerGUID, rank, false);
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
			ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARMY_RANK_INFO, rank);
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
				
			player->DestroyItemCount(item->GetEntry(), 1, true);

			ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARMY_RANK_LEVEL_UP, (playerRank + 1));
			return true;
		}
};

void AddSC_ArmyRank()
{
	new ArmyRank_Load_Config;
	new ArmyRank_Player_Script;
	new ArmyRank_Info_Show_Script;
	new ArmyRank_Up_Script;
}
