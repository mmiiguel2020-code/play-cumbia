#Requires AutoHotkey v2.0
#SingleInstance Force
#MaxThreadsPerHotkey 1
SendMode "Input"
SetKeyDelay -1, -1
SetTitleMatchMode 2

; =========================================================
; FL Studio — tecla física 9 + Play hold/gate (baja latencia)
;
;   Mantener 9 → Play + dispara C6 (tecla 0)
;   Soltar 9   → Stop
; Sin Sleep antes del sample: el C6 sale al instante.
; =========================================================

#HotIf WinActive("ahk_exe FL64.exe") || WinActive("ahk_class TFruityLoopsMainForm")

global busy9 := false

$9::
{
    global busy9
    if busy9
        return
    busy9 := true

    ; Sample YA (sin Sleep)
    SendInput "{Blind}{0 down}"
    ; Play justo después; re-assert inmediato por si Space pisa el hold
    ControlSend "{Space}", , "A"
    SendInput "{Blind}{0 down}"

    KeyWait "9"

    ControlSend "{Space}", , "A"
    SendInput "{Blind}{0 up}"
    busy9 := false
}

#HotIf

^!r::Reload()
^!q::ExitApp()

TrayTip("9 = sample C6 + Play (sin delay)`nCtrl+Alt+R = recargar", "PlayCumbia FL")
SetTimer(() => TrayTip(), -2500)
