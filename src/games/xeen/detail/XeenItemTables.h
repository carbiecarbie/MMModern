#ifndef MMODERN_XEEN_ITEM_TABLES_H
#define MMODERN_XEEN_ITEM_TABLES_H
// Numeric adaptation from ScummVM developers, GPL-3.0-or-later, without warranty.
// devtools/create_mm/create_xeen/constants.cpp at
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd. Preserve corresponding source
// and attribution to the upstream COPYRIGHT contributors in redistributions.
namespace mmodern::item_tables {
inline constexpr int weapons[]{0,50,15,100,80,40,60,1,10,150,30,60,8,50,100,15,30,15,200,80,250,150,400,100,40,120,300,100,200,300,25,100,50,15,0};
inline constexpr int armor[]{0,20,100,200,400,600,1000,2000,100,60,40,250,200,100};
inline constexpr int accessories[]{0,100,100,250,100,50,300,200,500,1000,2000};
inline constexpr int miscMaterial[]{0,50,1000,500,10,100,20,10,50,10,10,100,1,1,1,1,1,1,1,1,1,1};
inline constexpr int misc[]{0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,300,300,300,300,300,300,300,300,300,300,400,400,400,400,400,400,400,400,400,400,500,500,500,500,500,500,500,500,500,500,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600};
inline constexpr int metal[]{10,25,5,75,2,5,10,20,50,2,3,5,10,20,30,40,50,60,70,80,90,100};
inline constexpr int attributes[]{2,3,5,8,12,17,23,30,38,47,2,3,5,8,12,17,23,30,2,3,5,8,12,17,23,30,2,3,5,8,12,17,23,30,3,5,10,15,20,30,5,10,15,20,25,30,4,6,10,20,50,4,8,12,16,20,25,2,4,6,10,16,4,6,8,10,12,14,16,18,20,25};
inline constexpr int elemental[]{0,2,3,4,5,10,15,20,30,2,3,4,5,10,15,20,2,4,5,10,20,2,4,8,16,32,2,3,4,5,10,15,20,30,5,10,25};
inline constexpr int resistances[]{0,5,7,9,12,15,20,25,30,5,7,9,12,15,20,25,5,10,15,20,25,10,15,20,25,40,5,7,9,11,13,15,20,25,5,10,20};
inline constexpr int weaponBase[]{0,3,2,3,2,2,4,1,2,4,2,3,2,2,1,1,1,1,4,4,3,2,4,2,2,2,5,3,3,3,3,5,4,2,6};
inline constexpr int weaponMultiplier[]{0,3,3,4,5,4,2,3,3,3,3,3,2,4,10,6,8,9,4,3,6,8,5,6,4,5,3,5,6,7,2,2,2,2,4};
inline constexpr int metalDamage[]{-3,-6,-4,-2,2,4,6,8,10,0,1,1,2,2,3,4,5,12,15,20,30,50};
inline constexpr int metalHit[]{-3,-4,3,2,1,2,3,4,6,0,1,1,2,2,3,4,5,6,7,8,9,10};
inline constexpr int metalAc[]{-3,0,-2,-1,1,2,4,6,8,0,1,1,2,2,3,4,5,10,12,14,16,20};
inline constexpr int armorAc[]{0,2,4,5,6,7,8,10,4,2,1,1,1,1};
inline constexpr int miscSpells[]{76,42,1,45,26,27,20,61,59,10,62,60,38,72,55,50,40,2,4,73,41,75,3,34,43,31,52,49,44,53,30,33,74,28,15,25,5,9,0,70,17,7,69,24,8,29,51,14,66,12,13,6,23,57,22,71,64,56,21,11,48,46,54,19,36,32,58,18,47,37,65,35,63,16};
inline constexpr int attributeCategories[]{9,17,25,33,39,45,50,56,61,72};
inline constexpr int elementCategories[]{8,15,20,25,33,36};
}
#endif
