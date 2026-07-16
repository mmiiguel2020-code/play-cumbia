# Play Cumbia

App independiente de composición y análisis de audio (cumbia / regional mexicano).
No depende de Miguel Music Assistant ni de Star Cumbia: este es el repositorio propio.

## Qué incluye

- Generador de acordes y melodías
- Samples, afinador y efecto Bronco Max
- Secuenciador de ritmos (8 pistas) + piano Bajo Sexto
- EQ por sección y analizador de mezcla
- Persistencia de sesión (autosave)
- Script AutoHotkey v2 para FL Studio: `PlayCumbia-FL-Hold0.ahk`
  - Mantén **0** = play desde el inicio
  - Suelta **0** = parar

## Requisitos (Windows)

1. Visual Studio 2022+ con desarrollo C++
2. Git y CMake
3. AutoHotkey v2 (solo para el atajo de FL)

JUCE se descarga al configurar el proyecto.

## Compilar

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Atajo FL Studio

1. Instala [AutoHotkey v2](https://www.autohotkey.com/)
2. Ejecuta `PlayCumbia-FL-Hold0.ahk`
3. En FL: mantén `0` para reproducir desde el inicio; suelta para parar
4. `Ctrl+Alt+R` recarga el script · `Ctrl+Alt+Q` sale
