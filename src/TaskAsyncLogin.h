#ifndef TASK_ASYNC_LOGIN_H
#define TASK_ASYNC_LOGIN_H

#include <atomic>
#include <memory>
#include <optional>
#include "AsyncLogin.h"
#include "TokenFetcher.h"

// Le login en jeu, hors de la boucle de réception : chaque `Start()` lance une tâche FreeRTOS qui exécute l'appel bloquant de `TokenFetcher`
// (`HttpTokenFetcher` sur `ArduinoHttpPoster`) pendant que `loop()` continue de tourner. La tâche écrit son résultat dans un état partagé
// (`LoginRun`) que `Poll()` lit une fois `finished` posé ; `Abandon()` lâche cet état, la tâche finit seule et son résultat n'est lu par personne.
// Le `POST` d'un login abandonné n'est pas interrompu : il va au bout de son délai (celui du framework) et son résultat est jeté.
class TaskAsyncLogin : public AsyncLogin
{
public:
    struct LoginRun
    {
        std::atomic<bool> finished{false};
        LoginResult result{LoginStatus::Failed, ""};
    };

private:
    TokenFetcher &tokenFetcher;
    std::shared_ptr<LoginRun> current;

public:
    explicit TaskAsyncLogin(TokenFetcher &tokenFetcher) : tokenFetcher(tokenFetcher)
    {
    }

    void Start() override;
    std::optional<LoginResult> Poll() override;
    void Abandon() override;
};

#endif
