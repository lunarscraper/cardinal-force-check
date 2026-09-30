# cardinal-force-check

Machbarkeitstest: Läuft [DISTRHO Cardinal](https://github.com/DISTRHO/Cardinal) als VST2 auf der Akai Force?

1. **Actions → Cardinal-Check → Run workflow** (Tag leer lassen = neuestes Release).
2. Nach dem Lauf das Artefakt **cardinal-check** herunterladen und entpacken.
3. Weiter mit `ANLEITUNG.md` im entpackten Ordner.

Inhalt: `probe/probe.c` (Prüfprogramm: dlopen, optional Instanz + 10-s-Benchmark),
`check.sh` (Geräte-Infos, Bibliotheken-Abgleich, ruft das Prüfprogramm auf),
Workflow `.github/workflows/cardinal-check.yml`.

## Hinweis

Entwickelt mit Unterstützung von Claude (Anthropic)
