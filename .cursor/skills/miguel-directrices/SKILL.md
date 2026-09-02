---
name: miguel-directrices
description: >-
  Mandatory rules for this Bajxter/PICHADAW repo. Use at the start of every
  turn in play-cumbia. One repo folders, compile only the changed target, one
  JUCE fetch, verify audio before claiming done.
---

# Directrices

Leer al empezar el turno. Ejecutarlas.

- App: `Source/App/`. Plugins: `Source/Modules/`. Shared: `Source/Shared/`.
- Compilar solo el target: `.\compilar.ps1 app` | `delay` | `eq` | …
- JUCE una vez (FetchContent 8.0.14). No clonar por plugin.
- Verificar: compilar + medir audio. Un exe que abre no basta.
- El exe y el acceso se llaman **PICHADAW**. Copiar `PICHADAW.exe` a Escritorio y a `AppData/Local/Programs/Miguel Music Assistant/`.
- Español. Acordeón MIDI no va en este repo.
