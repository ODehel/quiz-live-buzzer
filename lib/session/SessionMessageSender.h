#ifndef SESSION_MESSAGE_SENDER_H
#define SESSION_MESSAGE_SENDER_H

// L'authentification et le renouvellement de la session. Les deux logins sont non bloquants (US-FW-005 CA-9, US-FW-007 CA-2) :
// `SendAuth` et `SendAuthRefresh` les lancent, `PollLogin` (à chaque tour de `loop()`) en recueille le résultat et envoie le message.
class SessionMessageSender
{
public:
    virtual ~SessionMessageSender() = default;
    // La connexion vient de s'ouvrir : un login frais, puis `auth`. Abandonne le login en vol, s'il y en a un.
    virtual void SendAuth() = 0;
    // `token_expiring_soon` : un login de renouvellement, puis `auth_refresh`. Sans effet si un login est déjà en vol.
    virtual void SendAuthRefresh() = 0;
    // Recueille le login terminé (ou dépassé), envoie ce qui doit l'être. Vrai si un `auth_refresh` est parti : le hub y répondra par un `auth_success`.
    virtual bool PollLogin() = 0;
    // La connexion qui a demandé le login en vol est fermée : il pourra se terminer, mais n'enverra rien.
    virtual void OnConnectionClosed() = 0;
    // Le buzzer est Éliminé : le login en vol est abandonné, son résultat jeté.
    virtual void Abandon() = 0;
};

#endif
