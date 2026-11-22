# US-FW-006 — Se reconnecter ou être éliminé

- **Couche** : logique
- **Statut** : Implémentée
- **Dépend du hub** : US-009 (codes de fermeture)

## Périmètre

Le compteur d'échecs de reconnexion consécutifs est armé au premier `auth_success` ; un échec l'incrémente, un succès le remet à 0, et le 3ᵉ échec consécutif fait passer en mode Éliminé, terminal (seul un redémarrage physique en sort). La temporisation et le déclenchement des tentatives sont délégués à la couche réseau.

## Sources

CDC §4.4, §5, §6, §9 (articulation avec le compteur) ; `lib/reconnection/`, `lib/connection/` ; `DEBTS.md` (« Reconnexion WebSocket sans recul sur refus du hub »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
