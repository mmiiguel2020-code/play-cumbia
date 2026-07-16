# Continuar Miguel Music Assistant

## Estado actual

La aplicación independiente está compilada e instalada en:

`C:\Users\MIGUEL\AppData\Local\Programs\Miguel Music Assistant`

El código fuente está en:

`C:\Users\MIGUEL\Documents\MiguelMusicAssistant`

**Importante:** Si la UI no refleja los cambios recientes, recompilar y copiar el `.exe` a la carpeta de instalación (ver abajo). El video de QA del 12/07/2026 pudo haberse grabado con una build anterior.

## Funciones terminadas (código fuente)

- Generador de acordes y melodías MIDI.
- Importación múltiple, drag-and-drop, reproducción, afinación y exportación de samples.
- Perilla de cambio de tono y medidor de aguja (`PrecisionRotarySlider`, sensibilidad 10× menor).
- Efecto Bronco Max.
- Creador de acordes Bajoquinto normal y bronco.
- Piano roll MIDI con acordes, melodía, zoom, scroll y playhead (`PianoRollView`).
- Secuenciador de 8 pistas con grid de 1/4 a 1/64 y hasta 256 pasos.
- EQ Low/Mid/High independiente por pista, incluida en exportación WAV.
- Bibliotecas locales de batería.
- Piano Bajo Sexto Bronco de 37 notas.
- Grabación, reproducción y exportación WAV.
- EQ gráfica interactiva de entrada y salida, con 7 bandas por sección (`GraphicEqDisplay`).
- Volumen y Bronco Max independientes por sección.
- Tema `MiguelLookAndFeel` con acentos por sección y layouts FlexBox parciales.
- Medidores VU estéreo, waveform, espectro FFT y correlación en Mezcla (`MixAnalyzerComponent`).
- Flag `/utf-8` en CMake para MSVC.

## QA validado en video (12/07/2026, ~8 min)

Referencia: `Grabación 2026-07-12 040043.mp4`

### Lo que funciona

| Área | Observación |
|------|-------------|
| **Mezcla** | VU L/R, waveform, espectro FFT y correlación responden en tiempo real. Sugerencias de nivel aparecen al reproducir. |
| **Generador** | Piano roll muestra acordes (púrpura) y melodía (azul). Playhead se mueve al pulsar Escuchar. |
| **Samples** | Importación múltiple, lista con botón ×, waveform, afinador con aguja y detección de nota/frecuencia. |
| **Ritmos y Piano** | Carga de samples por pista, grid con pasos activos, scroll horizontal, popup EQ por pista (Low/Mid/High), teclado Bronco responde. |
| **EQ 7 Bandas** | Curvas entrada/salida, nodos arrastrables sincronizados con sliders. |
| **Acordes Bajoquinto** | Selectores y reproducción de acordes WAV funcionan. |

### Problemas detectados en la build grabada

1. **Mojibake aún visible en runtime** — Ejemplos: `AnÃ¡lisis`, `CorrelaciÃ³n`, `melodÃ­a`, `grabaciÃ³n`, `TÃ³nica`, `SÃ­ntesis`, bullets `â€¢`. El código fuente ya incluye `/utf-8`; verificar que el ejecutable instalado sea la build recompilada.
2. **Fondos planos por pestaña** — Verde (Mezcla), azul (Generador), rosa (Ritmos), púrpura (Acordes). Falta unificar hacia el esquema carbón tipo FL Studio en todo el panel de contenido.
3. **Espacio vacío** — Pestañas Acordes Bajoquinto y partes de Generador/Samples dejan mucho área sin usar; layouts responsivos incompletos.
4. **Iconos corruptos** — Símbolos `â—¼` / `â˜¼` en headers de sección en lugar de flechas o bullets Unicode.
5. **Instalación vs build** — Si el `.exe` en `AppData\Local\Programs\` no se actualizó tras compilar, la UI seguirá mostrando la versión anterior.

## Carpetas importantes

- Tonos monofónicos:
  `C:\Users\MIGUEL\OneDrive2\Desktop\tonos`
- Motor de acordes:
  `C:\Users\MIGUEL\OneDrive2\Desktop\creador-acordes-bajoquinto`
- Piano Bronco:
  `C:\Users\MIGUEL\Documents\Miguel Music Assistant\Piano Bronco`
- Exportaciones:
  `C:\Users\MIGUEL\Documents\Miguel Music Assistant Exports`

## Para recompilar e instalar

```powershell
cmake --build build --config Release --parallel 4
Copy-Item "build\MiguelMusicAssistant_artefacts\Release\Standalone\Miguel Music Assistant.exe" `
  "$env:LOCALAPPDATA\Programs\Miguel Music Assistant\" -Force
```

## Próxima sesión

1. Validar que el mojibake desapareció y los fondos son carbón en todas las pestañas.
2. Probar layouts en 960×640, 1280×800 y 1920×1200 (Acordes en 2 columnas si ancho > 920 px).
3. Verificar VU, waveform, espectro, grid 1/64, EQ por pista e importación multiarchivo.
4. Confirmar que fold buttons muestran `[v]` / `[>]` sin caracteres corruptos.

## Cambios recientes (12/07/2026)

- `TabPagePanel`: fondo carbón con gradiente y línea de acento por pestaña (reemplaza colores planos).
- Caracteres Unicode problemáticos reemplazados (`•`, `·`, `▼`, `«»`) por ASCII seguro.
- Layout responsivo en Acordes Bajoquinto (2 columnas en pantallas anchas).
- Títulos en Generador y Samples; botones de Acordes con FlexBox.
- Build Release recompilada e instalada en `AppData\Local\Programs\Miguel Music Assistant\`.

## Archivos nuevos de esta iteración

- `Source/MiguelLookAndFeel.h/.cpp`
- `Source/PrecisionRotarySlider.h/.cpp`
- `Source/GraphicEqDisplay.h/.cpp`
- `Source/ImportedSampleList.h/.cpp`
- `Source/PianoRollView.h/.cpp`
- `Source/MixAnalyzerComponent.h/.cpp`

---

## Auditoría DAW (12/07/2026)

### Arquitectura actual

- **Audio RT:** `PluginProcessor::processBlock` → transport sample → EQ sección → suma → MIDI preview (sine synth) → `GrooveEngine` → `BroncoPianoEngine` → analizador (FIFO + atomics).
- **UI:** `PluginEditor.cpp` (~2010 líneas) monolito con 6 pestañas; componentes extraídos parcialmente (LookAndFeel, PianoRoll, EQ gráfico, analizador, lista samples).
- **Motores:** `GrooveEngine` (8×256 steps), `BroncoPianoEngine` (128 notas), `SectionEqBank` (5 secciones × 7 bandas).
- **Persistencia:** `getStateInformation` / `setStateInformation` son stubs (solo versión `"0.2.0"`).
- **Samples:** decode completo a RAM (`LoadedAudioSample`); sin caché compartida ni streaming.
- **Externo:** Python (`generar_acordes.py`, generación piano Bronco); rutas hardcodeadas a `C:\Users\MIGUEL\...`.

### Deuda técnica principal

1. Sin persistencia de sesión / proyecto.
2. `PluginEditor` god class (UI + DSP offline + Python + layout).
3. GrooveEngine procesa sample-a-sample con locks en audio thread.
4. Samples duplicados en RAM sin límite.
5. Rutas absolutas hardcodeadas (no portable).
6. Sin APVTS / automatización.
7. Sin Undo/Redo.
8. Piano roll solo lectura.
9. `MixAnalyzerComponent` repinta en cada bloque de audio.

### Roadmap priorizado (una mejora a la vez)

| Prioridad | Mejora | Riesgo | Impacto | Dificultad |
|-----------|--------|--------|---------|------------|
| **P0-1** | Persistencia sesión (`ValueTree`) | Crítico | Muy alto | Media-Alta |
| **P0-2** | Optimizar GrooveEngine (bloques, sin TryLock RT) | Alto | Muy alto | Media |
| **P0-3** | Caché LRU de samples + límite RAM | Alto | Alto | Media-Alta |
| **P1-4** | `AppPaths` / configuración centralizada | Alto | Alto | Baja |
| **P1-5** | Throttle analizador mezcla (repaint 30 Hz) | Medio | Medio | Baja |
| **P1-6** | APVTS para parámetros | Medio | Alto | Media |
| **P2-7** | Dividir editor por pestaña | Medio | Mantenibilidad | Media |
| **P2-8** | Undo/Redo grid + EQ | Medio | Alto UX | Media-Alta |
| **P2-9** | Atajos de teclado | Bajo | Medio | Baja |
| **P2-10** | Piano roll editable | Bajo | Alto | Alta |

### Primera mejora propuesta (pendiente de aprobación)

~~**P0-1 — Persistencia de sesión con `ValueTree`**~~ **Implementado 12/07/2026**

- Serializa: GrooveEngine (pattern, BPM, grid, gains, EQ por pista, paths de samples), SectionEqBank (5 secciones × 7 bandas + volumen/Bronco), UI (Generador, Samples importados, Ritmos, Acordes, pestaña activa).
- Autosave al cerrar en `%AppData%/Miguel Music Assistant/Sessions/autosave.mmas`.
- Restaura al abrir (autosave o estado del host vía `getStateInformation`).

**Siguiente mejora recomendada:** P0-3 — Caché LRU de samples + límite RAM.

### P0-2 — GrooveEngine optimizado (12/07/2026)

- Procesamiento por bloques con `processSamples` en EQ (en lugar de sample-a-sample).
- Snapshot del pattern/samples/gains: el audio thread no bloquea la UI durante todo el bloque.
- Si el lock falla, usa el último snapshot válido (ya no hay silencio total).
- Timing de steps por segmentos dentro del buffer de audio.

### Lo que NO tocar sin motivo

- FIFO analizador audio→UI.
- Separación processor / engines.
- Componentes UI reutilizables ya extraídos.
- Export WAV offline funcional.
