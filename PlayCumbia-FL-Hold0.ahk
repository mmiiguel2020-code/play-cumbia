#Requires AutoHotkey v2.0
#SingleInstance Force
#MaxThreadsPerHotkey 1
SendMode "Input"
SetKeyDelay -1, -1
SetTitleMatchMode 2

; =========================================================
; FL Studio: Mantener 0 = Play desde el inicio
;            Soltar 0   = Parar
; Sin Ctrl+Space. Solo Space (el atajo normal de FL).
; =========================================================

#HotIf WinActive("ahk_exe FL64.exe") || WinActive("ahk_class TFruityLoopsMainForm")

$0::
{
    ; Home + Space en el mismo buffer (sin Sleep → sin lag)
    SendInput "{Home}{Space}"
    KeyWait "0"
    ; Al soltar: Space para parar
    SendInput "{Space}"
}

#HotIf

; Recargar script
^!r::Reload()

; Salir
^!q::ExitApp()

TrayTip("Mantén 0 = play desde inicio`nSuelta 0 = para`nCtrl+Alt+R = recargar", "FL tecla 0")
SetTimer(() => TrayTip(), -2000)
