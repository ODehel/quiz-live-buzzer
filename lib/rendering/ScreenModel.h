#ifndef SCREEN_MODEL_H
#define SCREEN_MODEL_H

#include <string>
#include <variant>
#include <vector>

struct TextScreen
{
    std::string text;

    bool operator==(const TextScreen &other) const
    {
        return text == other.text;
    }
};

// Rien à redessiner : l'écran reste tel quel.
struct ScreenUnchanged
{
    bool operator==(const ScreenUnchanged &) const
    {
        return true;
    }
};

// Les propositions d'une question, une zone tactile par texte.
struct ZonesScreen
{
    std::vector<std::string> zoneTexts;

    bool operator==(const ZonesScreen &other) const
    {
        return zoneTexts == other.zoneTexts;
    }
};

// Un texte dans un bandeau, les dernières zones affichées dessous, la zone `zoneIndex` en évidence.
struct HighlightedZoneScreen
{
    size_t zoneIndex;
    std::string bannerText;

    bool operator==(const HighlightedZoneScreen &other) const
    {
        return zoneIndex == other.zoneIndex && bannerText == other.bannerText;
    }
};

// Un texte dans un bandeau, les dernières zones affichées dessous, la zone déjà choisie par le joueur toujours en évidence.
struct BannerScreen
{
    std::string bannerText;

    bool operator==(const BannerScreen &other) const
    {
        return bannerText == other.bannerText;
    }
};

// Les dernières zones affichées en plein écran, la zone `zoneIndex` (celle que le joueur a touchée) en évidence, sans texte.
struct ChosenZoneScreen
{
    size_t zoneIndex;

    bool operator==(const ChosenZoneScreen &other) const
    {
        return zoneIndex == other.zoneIndex;
    }
};

using ScreenModel = std::variant<ScreenUnchanged, TextScreen, ZonesScreen, HighlightedZoneScreen, BannerScreen, ChosenZoneScreen>;

#endif
