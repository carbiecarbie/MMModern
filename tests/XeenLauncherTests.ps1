param([string]$Launcher,[string]$Exe,[string]$GameDir,[string]$TestRoot,[string]$RomDir,[string]$PlayerExe)
$ErrorActionPreference='Stop'
$testBase=[IO.Path]::GetFullPath($TestRoot)
$TestRoot=Join-Path $testBase ([guid]::NewGuid().ToString('N'))
. $Launcher
function Check([bool]$Value,[string]$Message) { if (-not $Value) { throw $Message } }
Check ((Get-MMGameWindowHandle ([pscustomobject]@{ HasExited=$true; MainWindowHandle=[IntPtr]123 })) -eq [IntPtr]::Zero) 'Exited game has no activation handle'
Check ((Get-MMGameWindowHandle ([pscustomobject]@{ HasExited=$false; MainWindowHandle=$null })) -eq [IntPtr]::Zero) 'Missing window is never passed to native activation'
Check ((Get-MMGameWindowHandle ([pscustomobject]@{ HasExited=$false; MainWindowHandle=[IntPtr]123 })) -eq [IntPtr]123) 'Live game window handle'
$env:MMODERN_PLAYER_DIR=Join-Path $TestRoot 'player files'
$env:MMODERN_LOG_FILE=Join-Path $TestRoot 'logs\mmodern.log'
$env:MMODERN_MT32_ROM_DIR=$RomDir
$env:MMODERN_SETTINGS_FILE=Join-Path $TestRoot 'settings.ini'
New-Item -ItemType Directory -Force $env:MMODERN_PLAYER_DIR | Out-Null
function Get-MMHash([string]$Path) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($Path))) }
    finally { $sha.Dispose() }
}
$installedHash=Get-MMHash (Join-Path $GameDir 'WORLD\XEEN.CFG')
$owned=Join-Path $env:MMODERN_PLAYER_DIR 'XEEN.CFG'
[void](Invoke-MMCommand $Exe @('--sound-setup',$GameDir,'--reset'))
$version=Invoke-MMCommand $Exe @('--version')
Check ($version -match '^MMModern \d+\.\d+\.\d+\r?\n$') 'Version output'
$parsed=ConvertFrom-MMReport "version: MMModern test`r`nlog.file: C:\folder with spaces\logs\mmodern.log`r`nunknown: retained`n"
Check ($parsed['log.file'] -eq 'C:\folder with spaces\logs\mmodern.log') 'Report path parsing'
Check ($parsed['unknown'] -eq 'retained') 'Forward compatible parsing'
Check ((Get-MMGameArguments 'F:\Games\gog\Might and Magic 4-5') -eq '"F:\Games\gog\Might and Magic 4-5"') 'Player command line'
Check ((Get-MMGameArguments 'C:\ends in slash\') -eq '"C:\ends in slash\\"') 'Trailing slash quoting'
Check ((ConvertTo-MMArgument 'a"b') -eq '"a\"b"') 'Quote escaping'
$facts=@{ 'game.status'='usable'; 'game.edition'='World of Xeen (CD)'; 'game.folder'='F:\G'; 'game.exe'='usable for intros and Clouds ending'; 'cfg.source'='owned'
    'sound.music'='Roland LAPC-1 / MT-32; played'; 'sound.speech'='PC Speaker; silent'; 'roms.control'='missing'; 'roms.pcm'='missing'; 'roms.folder'='%LOCALAPPDATA%\MMModern\roms'; 'saves.used-slots'='0' }
$status=@(Get-MMStatusRows $facts)
Check (($status | ForEach-Object Title) -join ',' -eq 'Game,Music,Speech,MT-32 ROMs,Saves') 'Status rows'
Check ($status[1].Level -eq 'warn' -and $status[1].Detail -eq 'Needs your MT-32 ROMs') 'MT-32 without ROMs warns'
Check ($status[2].Level -eq 'warn' -and $status[2].Value -eq 'PC Speaker' -and $status[2].Detail -eq 'No sound in MMModern yet') 'Silent device warns by name'
Check ($status[3].Level -eq 'info' -and $status[4].Value -eq 'No saves yet') 'Missing ROMs are only information'
$facts['roms.control']='identified (MT-32 control 1.07)'; $facts['roms.pcm']='identified (MT-32 PCM)'; $facts['sound.speech']='None; silent'
$status=@(Get-MMStatusRows $facts)
Check ($status[1].Level -eq 'ok' -and $status[1].Detail -eq 'Chosen in Sound setup' -and $status[2].Level -eq 'ok' -and $status[3].Value -eq 'Found') 'Played devices and found ROMs'
Check ((@(Get-MMStatusRows @{ 'cfg.error'='Malformed' }) | Where-Object Title -eq 'Sound').Level -eq 'warn') 'Damaged owned CFG'
Check ((ConvertTo-MMVolumeLabel '1') -eq 'Yes' -and (ConvertTo-MMVolumeLabel '0') -eq 'No') 'Volume answer labels'
Check ((ConvertFrom-MMVolumeLabel 'Yes') -eq '1' -and (ConvertFrom-MMVolumeLabel 'No') -eq '0') 'Volume answer values'
$report=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',$GameDir))
Check ($report['game.status'] -eq 'usable') 'Original data detection'
Check ($report['cfg.source'] -eq 'installed') 'Installed CFG'
Check ($report['sound.music'] -eq 'Sound Blaster Pro; played') 'Installed SB Pro music'
Check ($report['sound.speech'] -eq 'Sound Blaster 16; played') 'Installed SB16 digital'
Check ($report['roms.control'] -eq 'identified (MT-32 control 1.07)') 'Control ROM identification'
Check ($report['roms.pcm'] -eq 'identified (MT-32 PCM)') 'PCM identification'
Check ($report['game.data-id'] -eq '0000000000cd0ddd-2d073b21-0000000000ac3f53-6421f2f8') 'Expected CD archive identity'
Check ($report['saves.folder'].EndsWith($report['game.data-id'])) 'Data id and saves directory'
# The known data id is independently checked against the existing save directory below.
$saveRoot=[Environment]::ExpandEnvironmentVariables($report['saves.folder'])
Check (Test-Path -LiteralPath $saveRoot -PathType Container) 'Existing original-data save directory'
$raw=Invoke-MMCommand $Exe @('--diagnostics',$GameDir)
Check (-not $raw.Contains($env:USERPROFILE)) 'Report profile redaction'
Check ($report['settings.file'] -eq $env:MMODERN_SETTINGS_FILE) 'Settings override'
$rules=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--sound-setup',$GameDir))
Check ($rules['speech.0.id'] -eq '6') 'PAS Alt writes digital 6'
$music=@(Get-MMDeviceChoices $rules 'music')
Check (($music | ForEach-Object Name) -join '|' -eq 'Adlib|Sound Blaster|Sound Blaster Pro|Pro-Audio Spectrum|Roland LAPC-1 / MT-32|None|PC Speaker|Roland Canvas / GS|Wave Blaster|Roland TAP-10') 'Music rows: played, None, then silent'
Check ($music.Count -eq 10 -and @($music | Where-Object Silent).Count -eq 4) 'Every installer music row stays selectable'
Check (($music | Where-Object Recommended).Label -eq 'Sound Blaster Pro (recommended)') 'Recommended music'
Check (($music | Where-Object Id -eq '4').Label -eq 'Roland LAPC-1 / MT-32 (needs MT-32 ROMs)') 'MT-32 names its ROMs'
$speech=@(Get-MMDeviceChoices $rules 'speech')
Check (($speech | ForEach-Object Name) -join '|' -eq 'Sound Blaster|Sound Blaster Pro|Sound Blaster 16|Pro-Audio Spectrum Alt|None|Adlib|PC Speaker|Pro-Audio Spectrum|Disney Sound Source|Covox Sound Master 2') 'Speech rows: played, None, then silent'
Check (($speech | Where-Object Recommended).Key -eq '9') 'Recommended speech is SB16'
Check ((Get-MMDeviceHint ($music | Where-Object Id -eq '4') 'music' $false) -like 'Put your MT-32*') 'MT-32 hint without ROMs'
Check ((Get-MMDeviceHint ($music | Where-Object Name -eq 'Wave Blaster') 'music' $true) -like 'MMModern does not play*') 'Silent device hint'
Check ($rules['music.0.music-port'] -eq '0x220,0x230,0x240,0x250,0x320,0x330,0x340,0x350') 'TAP-10 port list'
[void](Invoke-MMCommand $Exe @('--sound-setup',$GameDir,'--speech','9','--music','8','--digital-port','0x220','--irq','7','--dma','1','--volume','1'))
$mt32=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',$GameDir))
Check ($mt32['cfg.source'] -eq 'owned' -and $mt32['sound.music'] -eq 'Roland LAPC-1 / MT-32; played') 'Owned MT-32'
$override=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',$GameDir,'--xeen-cfg',(Join-Path $GameDir 'WORLD\XEEN.CFG')))
Check ($override['cfg.source'] -eq 'explicit' -and $override['sound.music'] -eq 'Sound Blaster Pro; played') 'Explicit CFG wins'
# Positive player-entry selection, then renderer refusal: no live UI and no save writes.
$env:SDL_VIDEODRIVER='dummy'
$env:SDL_AUDIODRIVER='dummy'
[void](Invoke-MMCommand $Exe @('--sound-setup',$GameDir,'--speech','1','--music','3'))
function Run-Player([string]$Folder,[string[]]$Overrides=@()) {
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName=$PlayerExe; $info.Arguments=(Get-MMGameArguments $Folder) + $(if ($Overrides.Count) { " " + (($Overrides | ForEach-Object { ConvertTo-MMArgument $_ }) -join " ") } else { "" })
    $info.UseShellExecute=$false; $info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
    $p=New-Object Diagnostics.Process; $p.StartInfo=$info
    try {
        [void]$p.Start(); $out=$p.StandardOutput.ReadToEndAsync(); $err=$p.StandardError.ReadToEndAsync()
        if (-not $p.WaitForExit(30000)) { $p.Kill(); throw 'Player test hung' }
        return @{ Code=$p.ExitCode; Text=$out.Result+$err.Result }
    } finally { $p.Dispose() }
}
$player=Run-Player $GameDir
Check ($player.Text -match 'Music device 7: not supported yet') 'Player audio read owned CFG'
$log=Get-Content -LiteralPath $env:MMODERN_LOG_FILE -Raw
Check ($log -match 'date: ' -and $log -match 'version: MMModern ' -and $log -match 'cfg.source: owned') 'Log header'
Check ($log -match 'Music device 7: not supported yet') 'Diagnostic also logged'
Check (-not $log.Contains($env:USERPROFILE)) 'Log profile redaction'
$failure=Run-Player (Join-Path $TestRoot 'missing game')
Check ($failure.Code -ne 0) 'Missing player folder fails'
Check ((Get-Content (Join-Path (Split-Path $env:MMODERN_LOG_FILE) 'mmodern-previous.log') -Raw) -eq $log) 'Previous log rotated'
[IO.File]::WriteAllText($owned,'bad')
$bad=Run-Player $GameDir
Check ($bad.Code -ne 0 -and $bad.Text.Contains($owned)) 'Malformed owned CFG stops player with filename'
$explicit=Run-Player $GameDir @('--xeen-cfg',(Join-Path $GameDir 'WORLD\XEEN.CFG'))
Check (-not $explicit.Text.Contains('Malformed XEEN.CFG') -and $explicit.Text.Contains('sound.music: Sound Blaster Pro; played')) 'Player explicit CFG overrides damaged owned file'
[IO.File]::WriteAllBytes($owned,[IO.File]::ReadAllBytes((Join-Path $GameDir 'WORLD\XEEN.CFG')))
$lock=[IO.File]::Open($owned,'Open','Read','None')
try {
    $unreadable=Run-Player $GameDir
    Check ($unreadable.Code -ne 0 -and $unreadable.Text.Contains($owned) -and $unreadable.Text.Contains('Cannot read')) 'Unreadable owned CFG stops with filename'
} finally { $lock.Dispose() }
$before=(Get-Item $env:MMODERN_LOG_FILE).LastWriteTimeUtc
$dev=Invoke-MMCommand $Exe @('--inspect-party',$GameDir)
Check ($dev.Length -gt 0) 'Developer ignores malformed owned CFG'
Check ((Get-Item $env:MMODERN_LOG_FILE).LastWriteTimeUtc -eq $before) 'Developer writes no log'
[void](Invoke-MMCommand $Exe @('--sound-setup',$GameDir,'--reset'))
Check (-not (Test-Path -LiteralPath $owned)) 'Reset removes malformed owned CFG'
$savedLog=$env:MMODERN_LOG_FILE
$blockedLog=Join-Path $TestRoot 'blocked-log'
New-Item -ItemType Directory -Force $blockedLog | Out-Null
$env:MMODERN_LOG_FILE=$blockedLog
$noLog=Run-Player $GameDir
Check ($noLog.Text -match 'Launcher test renderer refusal') 'Unwritable log does not prevent game entry'
Check (Test-Path -LiteralPath $blockedLog -PathType Container) 'Log rotation must not rename a directory'
$env:MMODERN_LOG_FILE=$savedLog
$missing=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',(Join-Path $TestRoot 'missing')))
Check ($missing['game.status'] -like 'unusable:*') 'Missing diagnostics is a report'
$env:MMODERN_MT32_ROM_DIR=Join-Path $TestRoot 'empty roms'
New-Item -ItemType Directory -Force $env:MMODERN_MT32_ROM_DIR | Out-Null
$wrongFile=Join-Path $env:MMODERN_MT32_ROM_DIR 'wrong.rom'
if (Test-Path -LiteralPath $wrongFile) { Remove-Item -LiteralPath $wrongFile }
$missingRom=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',$GameDir))
Check ($missingRom['roms.control'] -eq 'missing') 'Empty ROM folder'
[IO.File]::WriteAllText((Join-Path $env:MMODERN_MT32_ROM_DIR 'wrong.rom'),'wrong')
$wrong=ConvertFrom-MMReport (Invoke-MMCommand $Exe @('--diagnostics',$GameDir))
Check ($wrong['roms.control'] -like 'wrong*') 'Wrong ROM'
$script:MMState=@{Exe=$Exe}
Check (Test-GameFolder $GameDir) 'Launcher validation delegates to diagnostics'
Check (-not (Test-GameFolder (Join-Path $TestRoot 'missing'))) 'Launcher rejects unusable folder'
Check ((Get-MMHash (Join-Path $GameDir 'WORLD\XEEN.CFG')) -eq $installedHash) 'Installed CFG byte identity'
Write-Output 'Launcher helpers, diagnostics, player CFG selection and logs passed'

$resolved=[IO.Path]::GetFullPath($TestRoot)
if (-not $resolved.StartsWith($testBase+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test cleanup path' }
Remove-Item -LiteralPath $resolved -Recurse -Force
