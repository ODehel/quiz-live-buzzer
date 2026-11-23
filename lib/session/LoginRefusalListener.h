#ifndef LOGIN_REFUSAL_LISTENER_H
#define LOGIN_REFUSAL_LISTENER_H

// Prévenu quand le hub refuse le login redemandé pour une reconnexion (`401`) : le buzzer doit s'arrêter.
class LoginRefusalListener
{
public:
    virtual ~LoginRefusalListener() = default;
    virtual void OnLoginRefused() = 0;
};

#endif
