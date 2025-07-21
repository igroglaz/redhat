#ifndef CCHARACTER_HPP_INCLUDED
#define CCHARACTER_HPP_INCLUDED

#include <string>
#include <vector>

#include "BinaryStream.hpp"
#include "constants.h"

struct CEffect
{
    uint8_t Id1, Id2;
    uint8_t Value1, Value2;

    CEffect() : CEffect(0, 0, 0, 0) {
    }

    CEffect(uint8_t id1, uint8_t value1) : CEffect(id1, value1, 0, 0) {
    }

    CEffect(uint8_t id1, uint8_t value1, uint8_t id2, uint8_t value2) : Id1(id1), Id2(id2), Value1(value1), Value2(value2) {
    }

    bool operator==(const CEffect &other) const {
        return Id1 == other.Id1 && Id2 == other.Id2 && Value1 == other.Value1 && Value2 == other.Value2;
    }
};

struct CItem
{
    uint32_t Id;
    bool IsMagic;
    uint32_t Price;
    uint16_t Count;
    std::vector<CEffect> Effects;
};

class CItemList
{
    public:
        bool LoadFromStream(BinaryStream& stream);
        bool SaveToStream(BinaryStream& stream, bool min_format);

        uint8_t UnknownValue0,
                UnknownValue1,
                UnknownValue2;

        std::vector<CItem> Items;
};

class CCharacter
{
    public:
        CCharacter();
        ~CCharacter();

        bool LoadFromStream(BinaryStream& stream);
        bool SaveToStream(BinaryStream& stream);

        bool LoadFromFile(std::string filename);
        bool SaveToFile(std::string filename);
        uint16_t GenerateKey(uint16_t prev = 0);

        uint32_t StreamCRC2();
        uint32_t StreamCRC(BinaryStream& stream);

        void SaveSection(BinaryStream& file, uint32_t magic, uint16_t key, uint32_t crc, BinaryStream& section);

        bool IsWarrior() const { return Sex == sex::warrior; }
        bool IsMage() const { return Sex == sex::mage; }
        bool IsAmazon() const { return Sex == sex::amazon; }
        bool IsWitch() const { return Sex == sex::witch; }
        bool IsWizard() const { return Sex & sex::wizard; }
        bool IsFemale() const { return Sex & sex::female; }

        uint32_t TotalExperience() const {
            return ExpFireBlade + ExpWaterAxe + ExpAirBludgeon + ExpEarthPike + ExpAstralShooting;
        }

        uint32_t MonstersKills,
                 PlayersKills,
                 Frags,
                 Deaths,
                 Money,
                 Spells,
                 ActiveSpell,
                 ExpFireBlade,
                 ExpWaterAxe,
                 ExpAirBludgeon,
                 ExpEarthPike,
                 ExpAstralShooting,
                 Id1, Id2, HatId;

        int LoginID;
        int ID; // Unknown by the server. Populated within `Login_UpdateCharacter()`.

        uint8_t UnknownValue1,
                UnknownValue2,
                UnknownValue3,
                Picture,
                Body,
                Reaction,
                Mind,
                Spirit,
                Sex,
                MainSkill,
                Flags,
                Color;

        std::string Nick;
        std::string Clan;

        CItemList Bag;
        CItemList Dress;

        void CryptSection(BinaryStream& section, uint16_t key);

        std::string GetFullName();

        BinaryStream Section55555555;
        BinaryStream Section40A40A40;

        bool Retarded;

        std::string ClanTag;
};

inline bool IsIronMan(const CCharacter& chr) {
    return chr.Nick.length() && chr.Nick[0] == '@';
}

inline bool IsLegend(const CCharacter& chr) {
    return chr.Nick.length() && chr.Nick[0] == '_';
}

inline bool IsSolo(const CCharacter& chr) {
    return IsIronMan(chr) || IsLegend(chr);
}


#endif // CCHARACTER_HPP_INCLUDED
