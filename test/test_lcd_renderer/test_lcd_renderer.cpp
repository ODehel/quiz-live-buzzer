#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LcdRenderer.h"
#include "Screen.h"

class MockScreen : public Screen
{
public:
    MOCK_METHOD(void, Show, (const std::string &text), (override));
};

TEST(LcdRendererTest, AuthSuccessMessageShowsConnecteOnScreen)
{
    MockScreen mockScreen;
    LcdRenderer lcdRenderer{mockScreen};

    EXPECT_CALL(mockScreen, Show("Connecté")).Times(1);

    lcdRenderer.Apply("auth_success");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}