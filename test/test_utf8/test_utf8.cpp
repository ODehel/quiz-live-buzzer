#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "Utf8.h"

using Codes = std::vector<uint32_t>;

TEST(Utf8Test, AsciiCharacterShouldDecodeToItsOwnCode)
{
    EXPECT_EQ(Utf8::CodePoints("a"), (Codes{0x61}));
}

TEST(Utf8Test, TwoByteCharacterShouldDecodeToItsLatin1Code)
{
    EXPECT_EQ(Utf8::CodePoints("é"), (Codes{0xE9}));
}

TEST(Utf8Test, ThreeByteCharacterShouldDecodeToItsCode)
{
    EXPECT_EQ(Utf8::CodePoints("€"), (Codes{0x20AC}));
}

TEST(Utf8Test, FourByteCharacterShouldDecodeToItsCode)
{
    EXPECT_EQ(Utf8::CodePoints("😀"), (Codes{0x1F600}));
}

TEST(Utf8Test, LargestCodeOfEachSequenceLengthShouldDecodeToItself)
{
    EXPECT_EQ(Utf8::CodePoints("\xDF\xBF"), (Codes{0x07FF}));
    EXPECT_EQ(Utf8::CodePoints("\xEF\xBF\xBF"), (Codes{0xFFFF}));
    EXPECT_EQ(Utf8::CodePoints("\xF4\x8F\xBF\xBF"), (Codes{0x10FFFF}));
}

TEST(Utf8Test, MixedTextShouldDecodeCharacterByCharacterInOrder)
{
    EXPECT_EQ(Utf8::CodePoints("aé€b"), (Codes{0x61, 0xE9, 0x20AC, 0x62}));
}

TEST(Utf8Test, TruncatedSequenceAtTheEndShouldNotBeReadPastTheText)
{
    EXPECT_EQ(Utf8::CodePoints("a\xC3"), (Codes{0x61, 0xC3}));
}

TEST(Utf8Test, EmptyTextShouldDecodeToNothing)
{
    EXPECT_EQ(Utf8::CodePoints(""), (Codes{}));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
