#ifndef ARMYRANK_H
#define ARMYRANK_H

#include <unordered_map>

class ArmyRank
{

private:
    ArmyRank();
	~ArmyRank();

public:
	static ArmyRank* instance();

	// Players
		uint8 GetPlayerRank(uint64 v) { return PlayerRank[v]; }
		void SetPlayerRank(uint64 player_GUID, uint8 rank, bool update_DB);

	// Items
		uint8 GetItemRank(uint32 v) { return ItemRank[v]; }
		void SetItemRank(uint32 item_id, uint8 rank){ ItemRank[item_id] = rank; }


	// Public Tables
	std::unordered_map<uint64, uint8> PlayerRank;
	std::unordered_map<uint32, uint8> ItemRank;
};

#define sArmyRank ArmyRank::instance()
#endif
