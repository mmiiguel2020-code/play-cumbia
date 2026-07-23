#Requires AutoHotkey v2.0
#SingleInstance Force
#MaxThreadsPerHotkey 1
SendMode "Input"
SetKeyDelay -1, -1
SetTitleMatchMode 2

; =========================================================
; FL Studio — tecla física 9 + Play hold/gate
;
;   Mantener 9 → Play + dispara C6 (tecla 0 del typing keyboard)
;   Soltar 9   → Stop
;
; Por qué enviar 0 y no 9:
;   En el teclado piano de FL, 0 = C6 y 9 = A#5.
;   Antes, con la tecla 0, el doble disparo C6→C6 iba perfecto.
;   Con 9 se oía A#5→C6. Ahora la física es 9, la nota es C6.
; =========================================================

#HotIf WinActive("ahk_exe FL64.exe") || WinActive("ahk_class TFruityLoopsMainForm")

global busy9 := false

$9::
{
    global busy9
    if busy9
        return
    busy9 := true

    ; Sample en C6 (como cuando usábamos el 0)
    SendInput "{Blind}{0 down}"
    Sleep 50
    ControlSend "{Space}", , "A"
    Sleep 30
    ; Re-assert C6 (el mismo doble toque que antes funcionaba)
    SendInput "{Blind}{0 down}"

    KeyWait "9"

    ControlSend "{Space}", , "A"
    SendInput "{Blind}{0 up}"
    busy9 := false
}

#HotIf

^!r::Reload()
^!q::ExitApp()

TrayTip("9 = hold Play + sample C6`nCtrl+Alt+R = recargar", "PlayCumbia FL")
SetTimer(() => TrayTip(), -2500)
