#ifndef OBSERVABLE_WEB_SOCKETS_CLIENT_H
#define OBSERVABLE_WEB_SOCKETS_CLIENT_H

#include "WebSocketsClient.h"

// La bibliothèque WebSockets (2.7.x) n'émet aucun événement quand l'ouverture de la connexion TCP échoue
// (`connectFailedCb` est vide) : hub arrêté ou Wi-Fi coupé restent silencieux. Cette sous-classe lit l'état
// protégé de la bibliothèque pour le savoir.
class ObservableWebSocketsClient : public WebSocketsClient
{
public:
    // Une itération de la bibliothèque. Vrai si elle a tenté d'ouvrir la connexion TCP et n'y est pas arrivée.
    // `_lastConnectionFail` change à chaque ouverture tentée : horodaté à l'échec, remis à 0 à la réussite ;
    // il ne bouge pas quand `loop()` attend la fin de l'intervalle de reconnexion.
    bool LoopAndTellIfTheConnectFailed()
    {
        // Même vérification que `loop()` : si la connexion est tombée, elle émet `WStype_DISCONNECTED` et
        // horodate `_lastConnectionFail` ici, avant la lecture ci-dessous : une fermeture n'est pas un connect raté.
        const bool wasConnected = clientIsConnected(&_client);
        const unsigned long lastFailBefore = _lastConnectionFail;
        loop();
        return !wasConnected && _lastConnectionFail != lastFailBefore && !clientIsConnected(&_client);
    }
};

#endif
