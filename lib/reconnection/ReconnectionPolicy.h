#ifndef RECONNECTION_POLICY_H
#define RECONNECTION_POLICY_H
class ReconnectionPolicy
{
private:
    bool isArmed = false;
    bool isEliminated = false;
    bool isConnectionEstablished = false;
    int connectionFailuresCount = 0;

public:
    void OnAuthSuccess()
    {
        isArmed = true;
        isConnectionEstablished = true;
        connectionFailuresCount = 0;
    }

    void OnReconnectFailed()
    {
        if (isArmed)
        {
            connectionFailuresCount++;
            if (connectionFailuresCount >= 3)
                isEliminated = true;
        }
    }

    void OnConnectionLost()
    {
        if (isConnectionEstablished)
        {
            isConnectionEstablished = false;
            return;
        }
        OnReconnectFailed();
    }

    bool IsEliminated() const
    {
        return isEliminated;
    }
};

#endif