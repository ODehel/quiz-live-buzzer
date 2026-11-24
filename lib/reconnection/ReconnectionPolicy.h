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

    // Une première authentification a eu lieu : seule une connexion déjà établie peut être perdue (US-FW-015 CA-6).
    bool IsArmed() const
    {
        return isArmed;
    }

    // La tentative en cours : les échecs comptés + 1 (« Tentative N/3 », US-FW-015 CA-1). La perte de la connexion établie n'est pas un échec.
    int CurrentAttempt() const
    {
        return connectionFailuresCount + 1;
    }
};

#endif