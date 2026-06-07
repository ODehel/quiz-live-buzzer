#include <gtest/gtest.h>
#include "ReconnectionPolicy.h"

TEST(ReconnectionPolicyTest, UnarmedPolicySurvivesAFailure)
{
    ReconnectionPolicy policy;
    policy.OnReconnectFailed();
    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, ArmedPolicyIsEliminatedAfterThreeFailures)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    for (int i = 0; i < 3; i++)
    {
        policy.OnReconnectFailed();
    }
    EXPECT_TRUE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, AuthSuccessResetsTheFailureCount)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    for (int i = 0; i < 2; i++)
    {
        policy.OnReconnectFailed();
    }
    policy.OnAuthSuccess();
    policy.OnReconnectFailed();
    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, EliminatedPolicyStaysEliminatedAfterAuthSuccess)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    for (int i = 0; i < 3; i++)
    {
        policy.OnReconnectFailed();
    }
    policy.OnAuthSuccess();
    EXPECT_TRUE(policy.IsEliminated());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}