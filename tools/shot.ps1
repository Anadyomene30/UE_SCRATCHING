# scratchvj — une capture de la fenetre, parce qu'on ne peut pas la regarder.
#
# POURQUOI CE SCRIPT EXISTE. scratchvj_ui est une application WIN32 sans
# console : elle n'ecrit nulle part ce qu'elle affiche, et une session qui
# travaille sur l'interface sans pouvoir voir l'ecran livre des mises en page
# jamais regardees. Un equivalent de ce script a existe en septembre 2026, n'a
# jamais ete commite, et a disparu avec la session -- d'ou celui-ci, dans le
# depot.
#
# CE QU'IL NE FAIT PAS : cliquer. Les clics simules n'atteignent pas ImGui, qui
# lit son entree par SDL. Pour voir un ecran precis il faut donc le demander au
# lancement (--screen jouer|bibliotheque|effets|table|sortie|reglages), pas le
# chercher a la souris.
#
#   pwsh tools/shot.ps1 -Args "--screen table" -Out captures/table.png
#   pwsh tools/shot.ps1 -Args "--live clips/city_mask.mp4" -Wait 14

param(
    [string]$Exe = "build-ui/scratchvj/ui/Release/scratchvj_ui.exe",
    [string]$Arguments = "",
    [string]$Out = "capture.png",
    [int]$Wait = 10,
    [int]$Width = 1760,
    [int]$Height = 1000
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Shot {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y,
                                                                  int w, int t, bool repaint);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after,
                                                                    int x, int y, int w, int t,
                                                                    uint flags);
}
"@

# SANS CECI LA CAPTURE EST DECALEE. GetWindowRect rend des pixels physiques ;
# un processus PowerShell non conscient du DPI voit, lui, des coordonnees
# virtuelles mises a l'echelle. Sur un ecran a 125% les deux jeux different
# d'un quart, et l'image montre le bureau a cote de la fenetre -- ce qui a
# l'air d'une fenetre mal placee plutot que d'une mesure fausse.
[Win32Shot]::SetProcessDPIAware() | Out-Null

$root = (Get-Location).Path
$argList = if ($Arguments) { $Arguments -split ' +' } else { @() }
$proc = Start-Process -FilePath (Join-Path $root $Exe) -ArgumentList $argList `
                      -WorkingDirectory $root -PassThru

# Laisser l'application ouvrir sa fenetre, initialiser bgfx et, si un clip est
# passe en argument, l'analyser puis le charger : la capture d'une fenetre qui
# n'a pas fini de s'installer ne montre rien d'utile.
Start-Sleep -Seconds $Wait

if ($proc.HasExited) {
    Write-Output "l'application s'est arretee seule, code $($proc.ExitCode)"
    exit 1
}

$handle = $proc.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) {
    $proc.Refresh()
    $handle = $proc.MainWindowHandle
}
if ($handle -eq [IntPtr]::Zero) {
    Stop-Process -Id $proc.Id -Force
    Write-Output "aucune fenetre principale"
    exit 1
}

# Une taille et une place connues : l'interface a six ecrans et des panneaux
# qui se replient sous une certaine largeur, donc une capture faite a la taille
# ou la fenetre s'est ouverte ne dit pas ce que l'on croit.
[Win32Shot]::MoveWindow($handle, 0, 0, $Width, $Height, $true) | Out-Null

# Au premier plan, et TOPMOST plutot que SetForegroundWindow seul : Windows
# refuse le passage au premier plan demande par un processus qui n'a pas le
# focus, si bien que la capture montrait la fenetre qui recouvrait l'application
# plutot que l'application. HWND_TOPMOST (-1) n'a pas cette restriction.
[Win32Shot]::SetWindowPos($handle, [IntPtr](-1), 0, 0, $Width, $Height, 0x0040) | Out-Null
[Win32Shot]::SetForegroundWindow($handle) | Out-Null
Start-Sleep -Milliseconds 900

$rect = New-Object Win32Shot+RECT
[Win32Shot]::GetWindowRect($handle, [ref]$rect) | Out-Null
$w = $rect.R - $rect.L
$h = $rect.B - $rect.T

$bitmap = New-Object System.Drawing.Bitmap($w, $h)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen($rect.L, $rect.T, 0, 0, (New-Object System.Drawing.Size($w, $h)))

$outPath = Join-Path $root $Out
$outDir = Split-Path $outPath -Parent
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }
$bitmap.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bitmap.Dispose()

Stop-Process -Id $proc.Id -Force
Write-Output "$Out  ${w}x${h}"
