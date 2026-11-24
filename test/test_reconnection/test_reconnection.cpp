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

TEST(ReconnectionPolicyTest, LossesBeforeTheFirstAuthSuccessNeverEliminate)
{
    ReconnectionPolicy policy;
    for (int i = 0; i < 10; i++)
    {
        policy.OnConnectionLost();
    }
    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, LossOfTheEstablishedConnectionIsNotAFailure)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    policy.OnConnectionLost();
    for (int i = 0; i < 2; i++)
    {
        policy.OnConnectionLost();
    }
    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, ThirdFailureAfterTheLossOfTheEstablishedConnectionEliminates)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    for (int i = 0; i < 4; i++)
    {
        policy.OnConnectionLost();
    }
    EXPECT_TRUE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, AuthSuccessRearmsTheLossOfTheEstablishedConnection)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    for (int i = 0; i < 3; i++)
    {
        policy.OnConnectionLost();
    }
    policy.OnAuthSuccess();
    for (int i = 0; i < 3; i++)
    {
        policy.OnConnectionLost();
    }
    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ReconnectionPolicyTest, TheAttemptInProgressIsOneMoreThanTheCountedFailures)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    policy.OnConnectionLost();
    EXPECT_EQ(policy.CurrentAttempt(), 1);

    policy.OnReconnectFailed();
    EXPECT_EQ(policy.CurrentAttempt(), 2);

    policy.OnReconnectFailed();
    EXPECT_EQ(policy.CurrentAttempt(), 3);
}

TEST(ReconnectionPolicyTest, AuthSuccessBringsTheAttemptInProgressBackToOne)
{
    ReconnectionPolicy policy;
    policy.OnAuthSuccess();
    policy.OnReconnectFailed();
    policy.OnReconnectFailed();

    policy.OnAuthSuccess();

    EXPECT_EQ(policy.CurrentAttempt(), 1);
}

TEST(ReconnectionPolicyTest, IsArmedOnlyOnceAnAuthSuccessWasReceived)
{
    ReconnectionPolicy policy;
    EXPECT_FALSE(policy.IsArmed());

    policy.OnAuthSuccess();

    EXPECT_TRUE(policy.IsArmed());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}