#include "WebSocketConnectionCloser.h"
#include "WebSocketsClient.h"

// `disconnect()` envoie la fermeture (code 1000) si la connexion est ouverte et rend la main sans la rouvrir : la
// bibliothèque ne reconnecte que depuis `loop()`, qui n'est plus appelé une fois le réseau arrêté.
void WebSocketConnectionCloser::Close()
{
    client.disconnect();
}
