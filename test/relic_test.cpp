#include "UnitTest++.h"

#include "../circle.h"
#include "../relic.h"

namespace
{

TEST(IsRelic) {
    CItem non_relic{.Id=12345, .IsMagic=true, .Count=1, .Effects={{42, 1}}};
    CHECK_EQUAL(false, IsRelic(non_relic));

    CHECK_EQUAL(true, IsRelic(ascension_staff));
    CHECK_EQUAL(true, IsRelic(ascension_crown));

    for (const auto& [sex, items] : circle::AllAwards()) {
        for (const auto& item : items) {
            CHECK_EQUAL(true, IsRelic(item));
        }
    }
}

TEST(ReadSigil) {
    CItem non_relic{.Id=12345, .IsMagic=1, .Count=1, .Effects={{42, 1}}};
    CHECK_EQUAL(0, ReadSigil(non_relic));

    CItem staff{.Id=ascension_staff.Id, .IsMagic=1, .Count=1, .Effects={{stats::skill_blade, 1}, {stats::skill_axe, 2}, {stats::skill_bludgeon, 3}, {stats::skill_pike, 4}}};
    CHECK_EQUAL(0x04030201, ReadSigil(staff));

    // Check unrelated stats intertwined.
    CItem crown{.Id=ascension_crown.Id, .IsMagic=1, .Count=1, .Effects={
        {stats::body, 9}, {stats::skill_earth, 4}, {stats::skill_air, 3}, {stats::defence, 9}, {stats::skill_water, 2}, {stats::skill_fire, 1}, {stats::spirit, 9}},
    };
    CHECK_EQUAL(0x04030201, ReadSigil(crown));
    
    // Check a specific (realistically low) sigil.
    CItem low_sigil{.Id=18515, .IsMagic=1, .Count=1, .Effects={{stats::skill_fire, 5}}};
    CHECK_EQUAL(5, ReadSigil(low_sigil));
    
    // Check a specific sigil, a multiple of 256.
    CItem sigil_256{.Id=18515, .IsMagic=1, .Count=1, .Effects={{stats::skill_water, 5}}};
    CHECK_EQUAL(0x500, ReadSigil(sigil_256));
}

void CheckSerializableWithSigil(CItem item) {
    CHECK_EQUAL(true, EmbossSigil(item, 0xffffffff));
    CItemList list{.Items={item}};
    
    BinaryStream min_stream;
    CHECK_EQUAL(true, list.SaveToStream(min_stream, true));
    CItemList recoded_list;
    CHECK_EQUAL(true, recoded_list.LoadFromStream(min_stream));

    CHECK_EQUAL(1, recoded_list.Items.size());
    CHECK_EQUAL(item.Id, recoded_list.Items[0].Id);
    CHECK_EQUAL(item.IsMagic, recoded_list.Items[0].IsMagic);
    CHECK_EQUAL(item.Count, recoded_list.Items[0].Count);
    CHECK_EQUAL(item.Effects.size(), recoded_list.Items[0].Effects.size());
    for (size_t i = 0; i < item.Effects.size(); i++) {
        CHECK_EQUAL(item.Effects[i].Id1, recoded_list.Items[0].Effects[i].Id1);
        CHECK_EQUAL(item.Effects[i].Value1, recoded_list.Items[0].Effects[i].Value1);
        CHECK_EQUAL(item.Effects[i].Id2, recoded_list.Items[0].Effects[i].Id2);
        CHECK_EQUAL(item.Effects[i].Value2, recoded_list.Items[0].Effects[i].Value2);
    }

    BinaryStream full_stream;
    CHECK_EQUAL(true, list.SaveToStream(full_stream, false));
    CHECK_EQUAL(true, recoded_list.LoadFromStream(full_stream));

    CHECK_EQUAL(item.Id, recoded_list.Items[0].Id);
    CHECK_EQUAL(item.IsMagic, recoded_list.Items[0].IsMagic);
    CHECK_EQUAL(item.Count, recoded_list.Items[0].Count);
    CHECK_EQUAL(item.Effects.size(), recoded_list.Items[0].Effects.size());
    for (size_t i = 0; i < item.Effects.size(); i++) {
        CHECK_EQUAL(item.Effects[i].Id1, recoded_list.Items[0].Effects[i].Id1);
        CHECK_EQUAL(item.Effects[i].Value1, recoded_list.Items[0].Effects[i].Value1);
        CHECK_EQUAL(item.Effects[i].Id2, recoded_list.Items[0].Effects[i].Id2);
        CHECK_EQUAL(item.Effects[i].Value2, recoded_list.Items[0].Effects[i].Value2);
    }
}

TEST(EmbossSigil) {
    // Check that embossing a non-relic fails.
    CItem non_relic{.Id=12345, .IsMagic=1, .Count=1};
    CHECK_EQUAL(false, EmbossSigil(non_relic, 0x04030201));

    CItem item{.Id=ascension_crown.Id, .IsMagic=1, .Count=1};
    CHECK_EQUAL(true, EmbossSigil(item, 0x04030201));
    CHECK_EQUAL(4, item.Effects.size());
    CHECK_EQUAL(0x04030201, ReadSigil(item));

    // Check that embossing again doesn't overwrite existing sigil marks.
    CHECK_EQUAL(false, EmbossSigil(item, 0xFFFFFFFF));
    CHECK_EQUAL(4, item.Effects.size());
    CHECK_EQUAL(0x04030201, ReadSigil(item));

    // Check that embossing a multiple of 256 doesn't set the lower byte.
    CItem item2{.Id=ascension_staff.Id, .IsMagic=1, .Count=1};
    CHECK_EQUAL(true, EmbossSigil(item2, 0x500));
    CHECK_EQUAL(1, item2.Effects.size());
    CHECK_EQUAL(0x500, ReadSigil(item2));

    // Check that all relics can be serialized after embossing with a long sigil.
    CheckSerializableWithSigil(ascension_crown);
    CheckSerializableWithSigil(ascension_staff);

    for (const auto& [sex, items] : circle::AllAwards()) {
        for (const auto& item : items) {
            CheckSerializableWithSigil(item);
        }
    }
}

TEST(ClaimedRelics) {
    // Random character doesn't claim anything.
    CCharacter chr;
    chr.Nick = "nobody";
    chr.Sex = sex::warrior;
    chr.Reaction = 19;
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, ascension_crown.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 18515)); // First circle reward.

    // Ascended character claims the ascension crown.
    CHECK_EQUAL(2, ClaimedRelics(chr, 2, ascension_crown.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, ascension_staff.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, 18515));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, 17473));

    // Second circle character before nightmare claims first circle reward, but not second circle reward.
    chr.Nick = "2circle";
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 18515));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 19550));

    // Second circle character at nightmare claims both first and second circle rewards.
    chr.Reaction = 50;
    chr.Mind = 50;
    chr.Spirit = 50;
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 18515));
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 19550));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 19034));

    // Seventh circle character before nightmare: claims up to sixth circle reward, but not seventh circle reward.
    chr.Nick = "7circle";
    chr.Reaction = 49;
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 17730));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 17473));

    // Now all the same for a witch.
    chr.Sex = sex::witch;
    chr.Nick = "witch";
    chr.Reaction = 19;
    
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, ascension_staff.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 18515));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 55150));

    CHECK_EQUAL(2, ClaimedRelics(chr, 2, ascension_staff.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, ascension_crown.Id));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, 55150));
    CHECK_EQUAL(0, ClaimedRelics(chr, 2, 54885));

    chr.Nick = "2circle";
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 55150));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 56443));

    chr.Reaction = 50;
    chr.Mind = 50;
    chr.Spirit = 50;
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 55150));
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 56443));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 55927));

    chr.Nick = "7circle";
    chr.Reaction = 49;
    CHECK_EQUAL(1, ClaimedRelics(chr, 0, 54369));
    CHECK_EQUAL(0, ClaimedRelics(chr, 0, 54885));
}

}
