# PICHADAW

Taller de samples para Windows. **App** (`PICHADAW.exe`) y **plugin VST3** (`PICHADAW.vst3`) salen del mismo codigo.

**No es un DAW.** No sustituye a FL Studio. No es un plugin Fruity nativo.

Nota completa (instalacion, guardado, accesos): [`PICHADAW.txt`](PICHADAW.txt)  
Instalar copias en el PC, sin compilar: `.\instalar.ps1` o doble clic en `instalar.cmd`.

Repo: [mmiiguel2020-code/play-cumbia](https://github.com/mmiiguel2020-code/play-cumbia)

## Esto si

- Hasta 16 samples (wav, mp3, aif, flac, ogg): varios a la vez, mute, volumen
- Afinador + cambio de tono (0,5 cents por notch de rueda)
- EQ de 7 bandas sobre lo que suena
- Recorte inicio/final, fade, loop, reverse
- Rack propio: HP, LP, compresor, excitador, duplicador, distorsion, delay, reverb, volumen, velocity
- Piano de referencia C4–C7 (seno). MIDI 60 = **C4** = 261,63 Hz. FL suele etiquetar esa nota como C5
- REC de la mezcla (~30 s)
- VST3 **Synth / Instrument** para FL (Channel Rack). Tambien puede usarse como efecto

## Esto no

- Timeline de cancion, piano roll visible, generador MIDI en la UI (el codigo existe, no se muestra)
- Host VST3 de plugins ajenos
- Sampler con root note por archivo (el pitch estira el wav)
- Reverse / recorte por fila (son globales a lo que esta armado)
- Mac / iOS
- Acordeon MIDI (otro repo: `Documents/acordeon-midi`)

## De que esta hecha

C++20, **JUCE 8.0.14** (FetchContent una vez), CMake, Visual Studio 2022.

```
Source/App/       PICHADAW (UI, samples, afinador, EQ, REC, piano)
Source/Shared/    Rack, look, perillas (app y plugins)
Source/Modules/   VST3 Bajxter (FX sueltos para FL)
```

## Instalar en este PC

```powershell
cd Documents\play-cumbia
.\instalar.ps1
```

Copia el VST3 a `Common Files\VST3` y a Documentos, deja `PICHADAW.lnk` → Programas, y pone `PICHADAW.txt` en el Escritorio. **No** recompila y **no** pisa `PICHADAW-conservado`.

## Compilar (Windows)

Solo el target que cambiaste.

```powershell
.\compilar.ps1 app     # PICHADAW.exe → Escritorio y Programas (pisa el exe)
.\compilar.ps1 vst3    # solo el plugin
.\compilar.ps1 delay   # un FX Bajxter
```

`.\compilar.ps1 app` **pisa** el exe que te gusto. La copia fria esta en `Escritorio\PICHADAW-conservado`.

## Guardado (audio)

El REC escribe en `Documents\Miguel Music Assistant\Grabaciones`.  
La carpeta `Escritorio\grabaciones pichadaw` es tuya a mano; el codigo aun no graba ahi solo.

Autosave: `%APPDATA%\Miguel Music Assistant\Sessions\autosave.mmas`

## Accesos

Deja `PICHADAW.lnk`. Deja `PICHADAW-conservado`. Puedes borrar atajos viejos (Bajxterbeta, etc.).
