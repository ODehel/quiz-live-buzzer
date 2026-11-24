#ifndef LINK_STATE_DISPLAY_H
#define LINK_STATE_DISPLAY_H

// Dit au rendu que la liaison avec le hub est coupée (US-FW-009 CA-4, US-FW-015 CA-1) : une perte n'est pas un message du hub.
// `attempt` est la tentative en cours, de 1 à 3. Le sens inverse, « liée », est l'`auth_success` lui-même.
class LinkStateDisplay
{
public:
    virtual ~LinkStateDisplay() = default;
    virtual void ShowLinkLost(int attempt) = 0;
};

#endif
