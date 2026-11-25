#ifndef ASYNC_LOGIN_H
#define ASYNC_LOGIN_H

#include <optional>
#include "LoginResult.h"

// Un login `POST /api/v1/token` qui ne bloque pas l'appelant (US-FW-005 CA-9, US-FW-007 CA-2) : `Start()` rend la main aussitôt,
// `Poll()` rend le résultat une fois le login terminé (une seule fois), `Abandon()` en jette le résultat, s'il arrive un jour.
// `Start()` sur un login déjà en vol équivaut à `Abandon()` puis `Start()`. La borne de 5 s n'est pas ici : elle est tenue par l'appelant (`Clock`).
class AsyncLogin
{
public:
    virtual ~AsyncLogin() = default;
    virtual void Start() = 0;
    virtual std::optional<LoginResult> Poll() = 0;
    virtual void Abandon() = 0;
};

#endif
