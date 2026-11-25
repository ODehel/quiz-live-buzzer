#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "TaskAsyncLogin.h"

namespace
{
    // Pile de la tâche : l'appel `HTTPClient`, la lecture de la réponse et le décodage JSON du token.
    constexpr uint32_t LoginTaskStackBytes = 8192;
    constexpr UBaseType_t LoginTaskPriority = 1;

    struct LoginTaskArguments
    {
        TokenFetcher *tokenFetcher;
        std::shared_ptr<TaskAsyncLogin::LoginRun> run;
    };

    void RunLogin(void *parameter)
    {
        {
            std::unique_ptr<LoginTaskArguments> arguments{static_cast<LoginTaskArguments *>(parameter)};
            arguments->run->result = arguments->tokenFetcher->Fetch();
            arguments->run->finished.store(true);
        }
        vTaskDelete(nullptr);
    }
}

void TaskAsyncLogin::Start()
{
    current = std::make_shared<LoginRun>();
    LoginTaskArguments *arguments = new LoginTaskArguments{&tokenFetcher, current};
    if (xTaskCreate(RunLogin, "login", LoginTaskStackBytes, arguments, LoginTaskPriority, nullptr) != pdPASS)
    {
        // Pas de tâche : le login est un échec (`result` reste `Failed`), rendu au prochain `Poll()`.
        delete arguments;
        current->finished.store(true);
    }
}

std::optional<LoginResult> TaskAsyncLogin::Poll()
{
    if (!current || !current->finished.load())
        return std::nullopt;
    LoginResult result = current->result;
    current.reset();
    return result;
}

void TaskAsyncLogin::Abandon()
{
    current.reset();
}
