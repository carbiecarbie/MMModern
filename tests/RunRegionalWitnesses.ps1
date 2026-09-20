param(
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$Installation
)
$ErrorActionPreference='Stop'
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$game=(Resolve-Path -LiteralPath $Installation).Path
$exe=Join-Path $build 'mmodern_regional_cli_witness.exe'
$savedDriver=$env:SDL_VIDEODRIVER
$savedRenderer=$env:SDL_RENDER_DRIVER
$savedRoute=$env:MMODERN_REGION_ROUTE
function Invoke-Witness([string]$Name,[string]$Route,[string]$Save,[bool]$Resume) {
    $env:MMODERN_REGION_ROUTE=$Route
    if(-not $Resume){Copy-Item -LiteralPath (Join-Path $build 'm32-initial.mmsave') -Destination $Save}
    $arguments=@('--load-game',$game,$Save)
    & $exe @arguments > (Join-Path $build "m32-$Name.log") 2>&1
    if($LASTEXITCODE -ne 0){throw "Regional witness $Name failed; see m32-$Name.log"}
    Write-Output "PASS $Name"
}
try {
    & (Join-Path $build 'mmodern_regional_original.exe') $game (Join-Path $build 'm32-initial.mmsave') > (Join-Path $build 'm32-original.log') 2>&1
    if($LASTEXITCODE -ne 0){throw 'Legacy fixture/original controls failed'}
    $env:SDL_VIDEODRIVER='dummy'
    $env:SDL_RENDER_DRIVER='software'
    foreach($case in @(@('first','F'),@('east','RRFLFRFRFFRFRF'))) {
        $name=$case[0];$route=$case[1]
        $save=Join-Path $build "m32-$name.mmsave"
        Invoke-Witness "$name-produce" $route $save $false
        Invoke-Witness "$name-resume" '-' $save $true
        $continued=Join-Path $build "m32-$name-continued.mmsave"
        Copy-Item -LiteralPath $save -Destination $continued
        Invoke-Witness "$name-continue" 'LM' $continued $true
        $uninterrupted=Join-Path $build "m32-$name-uninterrupted.mmsave"
        Invoke-Witness "$name-uninterrupted" ($route+'LM') $uninterrupted $false
        if((Get-FileHash -LiteralPath $continued).Hash -ne (Get-FileHash -LiteralPath $uninterrupted).Hash){throw "$name continuation bytes differ"}
        Invoke-Witness "$name-restart" '-' $continued $true
    }
    Invoke-Witness 'resumed-ranged-stop' 'W' (Join-Path $build 'm32-first.mmsave') $true
    Invoke-Witness 'fresh-ranged-stop' 'FW' (Join-Path $build 'm32-first.mmsave') $false
    Invoke-Witness 'third-pulse-stop' 'LFFRF' (Join-Path $build 'm32-first.mmsave') $false
    Invoke-Witness 'automatic-sign' 'LFFFRFFFFRF' (Join-Path $build 'm32-sign.mmsave') $false
    Invoke-Witness 'sign-restart-no-replay' '-' (Join-Path $build 'm32-sign.mmsave') $true
    Invoke-Witness 'southern-stop' 'LFFFRFFFFRFRRFFF' (Join-Path $build 'm32-sign.mmsave') $false
    Write-Output 'All 16 separate-process regional SDL witnesses passed; both continued saves match uninterrupted bytes.'
} finally {
    $env:SDL_VIDEODRIVER=$savedDriver
    $env:SDL_RENDER_DRIVER=$savedRenderer
    $env:MMODERN_REGION_ROUTE=$savedRoute
}
