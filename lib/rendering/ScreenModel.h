#ifndef SCREEN_MODEL_H
#define SCREEN_MODEL_H

#include <cstddef>
#include <optional>
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
// `zoneTexts` : les producteurs le laissent vide, `RetainingScreen` le remplit des dernières propositions affichées (US-FW-015 CA-2).
struct HighlightedZoneScreen
{
    size_t zoneIndex;
    std::string bannerText;
    std::vector<std::string> zoneTexts = {};

    bool operator==(const HighlightedZoneScreen &other) const
    {
        return zoneIndex == other.zoneIndex && bannerText == other.bannerText && zoneTexts == other.zoneTexts;
    }
};

// Un texte dans un bandeau, les dernières zones affichées dessous, la zone déjà choisie par le joueur toujours en évidence.
// `zoneTexts` et `chosenZone` : laissés vides par les producteurs, remplis par `RetainingScreen`.
struct BannerScreen
{
    std::string bannerText;
    std::vector<std::string> zoneTexts = {};
    std::optional<size_t> chosenZone = std::nullopt;

    bool operator==(const BannerScreen &other) const
    {
        return bannerText == other.bannerText && zoneTexts == other.zoneTexts && chosenZone == other.chosenZone;
    }
};

// Les dernières zones affichées en plein écran, la zone `zoneIndex` (celle que le joueur a touchée) en évidence, sans texte.
// `zoneTexts` : laissé vide par les producteurs, rempli par `RetainingScreen`.
struct ChosenZoneScreen
{
    size_t zoneIndex;
    std::vector<std::string> zoneTexts = {};

    bool operator==(const ChosenZoneScreen &other) const
    {
        return zoneIndex == other.zoneIndex && zoneTexts == other.zoneTexts;
    }
};

// « Hors jeu » (CDC §8.2, US-FW-015 CA-8) : un anneau avec le symbole marche/arrêt, le titre, la cause et la seule sortie, sans zones.
struct OutOfGameScreen
{
    std::string title;
    std::string cause;
    std::string wayOut;

    bool operator==(const OutOfGameScreen &other) const
    {
        return title == other.title && cause == other.cause && wayOut == other.wayOut;
    }
};

// Ce que l'écran de coupure atténue : le dernier écran affiché, sans son texte d'en-tête (le bandeau rouge en prend la place).
// Un texte plein écran, ou des propositions dont l'une peut être en évidence ; tout vide : rien à atténuer.
struct DimmedScreen
{
    std::string text = "";
    std::vector<std::string> zoneTexts = {};
    std::optional<size_t> highlightedZone = std::nullopt;

    bool operator==(const DimmedScreen &other) const
    {
        return text == other.text && zoneTexts == other.zoneTexts && highlightedZone == other.highlightedZone;
    }
};

// La coupure de la connexion établie (US-FW-015 CA-1, CA-2) : le bandeau rouge « Connexion perdue · Tentative N/3 » sous le bandeau du haut,
// le dernier écran atténué dessous. `dimmed` et `bannerOnly` : laissés par les producteurs, remplis par `RetainingScreen` ;
// `bannerOnly` : la coupure était déjà à l'écran, seul le bandeau est repeint.
struct ConnectionLostScreen
{
    std::string title;
    std::string attemptLabel;
    int attempt;      // les points pleins
    int attemptCount; // tous les points
    DimmedScreen dimmed = {};
    bool bannerOnly = false;

    bool operator==(const ConnectionLostScreen &other) const
    {
        return title == other.title && attemptLabel == other.attemptLabel && attempt == other.attempt && attemptCount == other.attemptCount &&
               dimmed == other.dimmed && bannerOnly == other.bannerOnly;
    }
};

using ScreenModel = std::variant<ScreenUnchanged, TextScreen, ZonesScreen, HighlightedZoneScreen, BannerScreen, ChosenZoneScreen, OutOfGameScreen, ConnectionLostScreen>;

#endif
