/*
  Delocto Chess Engine
  Copyright (c) 2018 Moritz Terink

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <random>
#include "magic.hpp"

// Magic numbers for rooks
const uint64_t magicNumberRook[64] = {

    36028866279506048, 18014467231055936, 36037661833560192, 36033229426262144, 36031013222613120, 36029905120788608, 36029346791555584, 36028935540048128,
    140738029420672, 70369281069056, 140806209929344, 140771849142400, 140754668748928, 140746078552192, 140741783453824, 140738570486016,
    35734132097152, 70643624185856, 141287512612864, 141287378391040, 141287311280128, 141287277724672, 1103806726148, 2199027482884,
    35736275402752, 35185445851136, 17594335625344, 8798241554560, 4400194519168, 2201171001472, 1101667500544, 140739635855616,
    35459258384512, 35185450029056, 17594341924864, 8798248898560, 4400202385408, 2201179128832, 2199040098308, 140738570486016,
    35459258417152, 35185445863552, 17592722948224, 8796361490560, 4398180761728, 2199090397312, 1099545215104, 277042298884,
    35459258384512, 35185445863552, 17592722948224, 8796361490560, 4398180761728, 2199090397312, 140741783453824, 140738578874496,
    72054149422608714, 36025352403644746, 18014183758528662, 4432674693377, 281483634278417, 281483633756161, 281477157749761, 562927068959138

};

// Magic numbers for bishops
const uint64_t magicNumberBishop[64] = {

    565157600297472, 565157600296960, 1127008041959424, 1130300100837376, 299342040662016, 143006304829440, 71485708304384, 17871427092480,
    4415293752320, 2207646876160, 4402375163904, 4415234768896, 1169304846336, 558618378240, 279241048064, 139620524032,
    1125934401325056, 562967200662528, 281483600331264, 140771881664512, 140754678710272, 35188675985408, 70370925748224, 35185462874112,
    571746315931648, 285873157965824, 35734195078144, 70643689259520, 145135543263232, 70643655708672, 141287261212672, 70643630606336,
    285941744803840, 142970872401920, 17884244346880, 2201171263616, 70644696088832, 141291539267840, 282578783438848, 141289391719424,
    142971408236544, 71485704118272, 8935813681152, 137724168192, 8800392184832, 282578800083456, 565157600166912, 282578800083456,
    71485708304384, 35742854152192, 139654594560, 545783808, 68753162240, 4415360729088, 1130315200593920, 565157600296960,
    17871427092480, 139620524032, 545525760, 2131968, 268567040, 17247502848, 4415293752320, 565157600297472

};

// Occupancy masks for rooks
const uint64_t occupancyMaskRook[64] = {

    282578800148862, 565157600297596, 1130315200595066, 2260630401190006, 4521260802379886, 9042521604759646, 18085043209519166, 36170086419038334,
    282578800180736, 565157600328704, 1130315200625152, 2260630401218048, 4521260802403840, 9042521604775424, 18085043209518592, 36170086419037696,
    282578808340736, 565157608292864, 1130315208328192, 2260630408398848, 4521260808540160, 9042521608822784, 18085043209388032, 36170086418907136,
    282580897300736, 565159647117824, 1130317180306432, 2260632246683648, 4521262379438080, 9042522644946944, 18085043175964672, 36170086385483776,
    283115671060736, 565681586307584, 1130822006735872, 2261102847592448, 4521664529305600, 9042787892731904, 18085034619584512, 36170077829103616,
    420017753620736, 699298018886144, 1260057572672512, 2381576680245248, 4624614895390720, 9110691325681664, 18082844186263552, 36167887395782656,
    35466950888980736, 34905104758997504, 34344362452452352, 33222877839362048, 30979908613181440, 26493970160820224, 17522093256097792, 35607136465616896,
    9079539427579068672, 8935706818303361536, 8792156787827803136, 8505056726876686336, 7930856604974452736, 6782456361169985536, 4485655873561051136, 9115426935197958144

};

// Occupancy masks for bishops
const uint64_t occupancyMaskBishop[64] = {

    18049651735527936, 70506452091904, 275415828992, 1075975168, 38021120, 8657588224, 2216338399232, 567382630219776,
    9024825867763712, 18049651735527424, 70506452221952, 275449643008, 9733406720, 2216342585344, 567382630203392, 1134765260406784,
    4512412933816832, 9024825867633664, 18049651768822272, 70515108615168, 2491752130560, 567383701868544, 1134765256220672, 2269530512441344,
    2256206450263040, 4512412900526080, 9024834391117824, 18051867805491712, 637888545440768, 1135039602493440, 2269529440784384, 4539058881568768,
    1128098963916800, 2256197927833600, 4514594912477184, 9592139778506752, 19184279556981248, 2339762086609920, 4538784537380864, 9077569074761728,
    562958610993152, 1125917221986304, 2814792987328512, 5629586008178688, 11259172008099840, 22518341868716544, 9007336962655232, 18014673925310464,
    2216338399232, 4432676798464, 11064376819712, 22137335185408, 44272556441600, 87995357200384, 35253226045952, 70506452091904,
    567382630219776, 1134765260406784, 2832480465846272, 5667157807464448, 11333774449049600, 22526811443298304, 9024825867763712, 18049651735527936

};

// Magic shifts for rooks
const int magicNumberShiftsRook[64] = {

    52, 53, 53, 53, 53, 53, 53, 52,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 53, 53, 53, 53, 53

};

// Magic shifts for bishops
const int magicNumberShiftsBishop[64] = {

    58, 59, 59, 59, 59, 59, 59, 58,
	59, 59, 59, 59, 59, 59, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 59, 59, 59, 59, 59, 59,
	58, 59, 59, 59, 59, 59, 59, 58

};

// Occupancy variations
uint64_t occupancyVariation[64][4096];

// Final databases
uint64_t magicMovesRook[64][4096];
uint64_t magicMovesBishop[64][4096];

// Get vector of set bits in bitboard
static std::vector<int> getSetBits(uint64_t b) {
    std::vector<int> vector;
    int count;

    for (count = 63; count >= 0; count--) {
        if (SQUARES[count] & b) {
            vector.push_back((63 - count));
        }
    }

    return vector;

}

// Code taken from http://www.afewmorelines.com/understanding-magic-bitboards-in-chess-programming/ (modified version)
// Generate all possible occupancy variations for rook/bishop for each square
void generateOccupancyVariations(bool isRook) {
    int i, j, sq;
    uint64_t mask;
    int variationCount;
    std::vector<int> setBitsInMask, setBitsInIndex;
    int bitCount[64];

    for (sq=0; sq<=63; sq++) {
        mask = isRook ? occupancyMaskRook[sq] : occupancyMaskBishop[sq];
        setBitsInMask = getSetBits(mask);
        bitCount[sq] = popcount(mask);
        variationCount = (1ULL << bitCount[sq]);
        for (i=0; i<variationCount; i++) {

            occupancyVariation[sq][i] = 0;

            // find bits set in index "i" and map them to bits in the 64 bit "occupancyVariation"

            setBitsInIndex = getSetBits(i); // an array of integers showing which bits are set
            for (j=0; j < setBitsInIndex.size(); j++) {
                occupancyVariation[sq][i] |= (1ULL << setBitsInMask[setBitsInIndex[j]]);
            }

        }
    }
}

// Generate move-databases
void generateMoveDatabase(bool isRook) {
    uint64_t validMoves;
    int variations, bitCount;
    int sq, i, j, magicIndex;

    for (sq=0; sq<=63; sq++) {
        bitCount = isRook ? popcount(occupancyMaskRook[sq]) : popcount(occupancyMaskBishop[sq]);
        variations = (1ULL << bitCount);

        for (i=0; i<variations; i++) {
            validMoves = 0;
            if (isRook) {
                magicIndex = ((occupancyVariation[sq][i] * magicNumberRook[sq]) >> magicNumberShiftsRook[sq]);

                for (j=sq+8; j<=63; j+=8) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq-8; j>=0; j-=8) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq+1; j%8!=0; j++) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq-1; j%8!=7 && j>=0; j--) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }

                magicMovesRook[sq][magicIndex] = validMoves;
            } else {
                magicIndex = ((occupancyVariation[sq][i] * magicNumberBishop[sq]) >> magicNumberShiftsBishop[sq]);

                for (j=sq+9; j%8!=0 && j<=63; j+=9) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq-9; j%8!=7 && j>=0; j-=9) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq+7; j%8!=7 && j<=63; j+=7) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }
                for (j=sq-7; j%8!=0 && j>=0; j-=7) { validMoves |= (1ULL << j); if ((occupancyVariation[sq][i] & (1ULL << j)) != 0) break; }

                magicMovesBishop[sq][magicIndex] = validMoves;
            }
        }
    }
}
