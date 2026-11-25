#ifndef SESSION_POLLER_H
#define SESSION_POLLER_H

// Recueille, à chaque tour de `loop()`, le login en vol de la session (US-FW-005 CA-9, US-FW-007 CA-2).
class SessionPoller
{
public:
    virtual ~SessionPoller() = default;
    virtual void PollSession() = 0;
};

#endif
