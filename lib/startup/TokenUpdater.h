#ifndef TOKEN_UPDATER_H
#define TOKEN_UPDATER_H

#include <string>

// Reçoit le token du login du démarrage : c'est celui du premier `auth` si le login frais de l'ouverture n'aboutit pas.
class TokenUpdater
{
public:
    virtual ~TokenUpdater() = default;
    virtual void UpdateToken(const std::string &token) = 0;
};

#endif
