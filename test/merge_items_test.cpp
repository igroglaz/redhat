#include <string>

#include "UnitTest++.h"

#include "../merge_items.hpp"
#include "../login.hpp"

namespace
{

TEST(MergeItems_EmptyBag) {
    CItemList items = Login_UnserializeItems("[0,0,0,0]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,0]", got);
}

TEST(MergeItems_NoChanges) {
    CItemList items = Login_UnserializeItems("[0,0,0,2];[3667,0,0,1];[1000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,2];[3667,0,0,1];[1000,0,2,1]", got);
}

TEST(MergeItems_NonQuestItem) {
    CItemList items = Login_UnserializeItems("[0,0,0,3];[1000,0,31415,1];[1000,0,31415,1];[1000,0,31415,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,3];[1000,0,31415,1];[1000,0,31415,1];[1000,0,31415,1]", got);
}

TEST(MergeItems_DifferentItems) {
    CItemList items = Login_UnserializeItems("[0,0,0,3];[1000,0,2,1];[2000,0,2,1];[3000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,3];[1000,0,2,1];[2000,0,2,1];[3000,0,2,1]", got);
}

TEST(MergeItems_OnlyUpgrade) {
    CItemList items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:1:0:0}]", got);
}

TEST(MergeItems_UpgradeInTheMiddle) {
    CItemList items = Login_UnserializeItems("[0,0,0,5];[1000,0,2,1];[2000,0,2,1];[2000,0,2,1];[2000,0,2,1];[3000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,4];[1000,0,2,1];[2000,1,2,1,{7:1:0:0}];[2000,0,2,1];[3000,0,2,1]", got);
}

TEST(MergeItems_TwoUpgrades) {
    CItemList items = Login_UnserializeItems("[0,0,0,6];[1000,0,2,1];[2000,1,2,1,{5:10:0:0}];[2000,1,2,1,{5:10:0:0}];[3000,0,2,1];[3000,0,2,1];[4000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,4];[1000,0,2,1];[2000,1,2,1,{5:10:0:0},{7:1:0:0}];[3000,1,2,1,{7:1:0:0}];[4000,0,2,1]", got);
}

TEST(MergeItems_FourItems) {
    CItemList items = Login_UnserializeItems("[0,0,0,4];[1000,0,2,1];[1000,0,2,1];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,2];[1000,1,2,1,{7:1:0:0}];[1000,1,2,1,{7:1:0:0}]", got);
}

TEST(MergeItems_ImproveExisting) {
    CItemList items = Login_UnserializeItems("[0,0,0,2];[1000,1,2,1,{7:100:0:0}];[1000,1,2,1,{7:100:0:0}]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:101:0:0}]", got);
}

TEST(MergeItems_Improve255) {
    CItemList items = Login_UnserializeItems("[0,0,0,2];[1000,1,2,1,{7:255:0:0}];[1000,1,2,1,{7:255:0:0}]");
    MergeItems(items, 2);
    std::string got = Login_SerializeItems(items);

    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:255:0:0},{7:1:0:0}]", got);
}

TEST(MergeItems_ServerID) {
    CItemList items;

    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 2);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:1:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 3);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:2:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 4);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:2:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 5);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:2:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 6);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:2:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 7);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:2:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 8);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:4:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 9);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:3:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 10);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:3:0:0}]", Login_SerializeItems(items));
    
    items = Login_UnserializeItems("[0,0,0,2];[1000,0,2,1];[1000,0,2,1]");
    MergeItems(items, 11);
    CHECK_EQUAL("[0,0,0,1];[1000,1,2,1,{7:3:0:0}]", Login_SerializeItems(items));
}

}
