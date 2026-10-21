# Packages the tested build-rel executable as a Windows release zip.
# The executable is stripped into a staging copy; the build tree is untouched.
param(
	[string]$BuildDir = 'build-rel',
	[Parameter(Mandatory = $true)][string]$OutputDir,
	[string]$Ucrt64 = 'C:\msys64\ucrt64\bin',
	[string]$Ldd = 'C:\msys64\usr\bin\ldd.exe'
)
$ErrorActionPreference = 'Stop'
$repository = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = ([regex]::Match((Get-Content (Join-Path $repository 'CMakeLists.txt') -Raw), 'project\(MMModern VERSION ([0-9.]+)')).Groups[1].Value
$scummvm = ([regex]::Match((Get-Content (Join-Path $repository 'docs/dependencies.md') -Raw), 'checkout --detach ([0-9a-f]{40})')).Groups[1].Value
if (-not $version -or -not $scummvm) { throw 'Cannot read the project version or pinned ScummVM revision' }
$cacheText = Get-Content (Join-Path (Join-Path $repository $BuildDir) 'CMakeCache.txt') -Raw
$scummvmSource = ([regex]::Match($cacheText, '(?m)^SCUMMVM_SOURCE_DIR:PATH=(.+)\r?$')).Groups[1].Value.Trim()
if (-not (Test-Path (Join-Path $scummvmSource 'LICENSES/COPYING.LGPL'))) { throw 'Cannot find the pinned Nuked LGPL license' }
$output = [IO.Path]::GetFullPath($OutputDir)
if ($output.StartsWith($repository, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be outside the repository' }
$name = "MMModern-$version-windows-x64"
$zip = Join-Path $output "$name.zip"
$stage = Join-Path $output $name
if ((Test-Path $zip) -or (Test-Path $stage)) { throw "Output already exists: $zip or $stage" }
$exe = Join-Path (Resolve-Path (Join-Path $repository $BuildDir)).Path 'mmodern.exe'
New-Item -ItemType Directory -Force $stage | Out-Null

& (Join-Path $Ucrt64 'strip.exe') --strip-debug -o (Join-Path $stage 'mmodern.exe') $exe
if ($LASTEXITCODE) { throw 'strip failed' }
# Copy exactly the non-system DLLs ldd resolves, and only from UCRT64.
$dlls = & $Ldd (Join-Path $stage 'mmodern.exe') | ForEach-Object {
	if ($_ -match '^\s*(\S+\.dll) => (\S+)') {
		if ($Matches[2] -like '/ucrt64/bin/*') { $Matches[1] }
		elseif ($Matches[2] -notlike '/c/WINDOWS/*') { throw "Unexpected runtime dependency: $_" }
	}
}
if ($LASTEXITCODE) { throw 'ldd failed' }
foreach ($dll in $dlls) { Copy-Item (Join-Path $Ucrt64 $dll) $stage }

Copy-Item (Join-Path $repository 'LICENSE') (Join-Path $stage 'LICENSE.txt')
Copy-Item (Join-Path $repository 'README.md') (Join-Path $stage 'README.md')
# Upstream license texts as installed by MSYS2, next to the libraries they cover.
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory $licenses | Out-Null
Copy-Item (Join-Path $scummvmSource 'LICENSES/COPYING.LGPL') (Join-Path $licenses 'Nuked-LGPL.txt')
$share = Join-Path (Split-Path $Ucrt64) 'share\licenses'
foreach ($pair in @(@('SDL2\LICENSE.txt', 'SDL2.txt'), @('zlib\LICENSE', 'zlib.txt'),
		@('winpthreads\COPYING', 'winpthreads.txt'), @('gcc-libs\COPYING.RUNTIME', 'GCC-runtime-exception.txt'))) {
	Copy-Item (Join-Path $share $pair[0]) (Join-Path $licenses $pair[1])
}
$pacman = Join-Path (Split-Path $Ldd) 'pacman.exe'
$packages = (& $pacman -Q mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-libwinpthread mingw-w64-ucrt-x86_64-gcc) -join "`r`n  "
if ($LASTEXITCODE) { throw 'pacman version query failed' }
@"
MMModern $version - third-party notices

MMModern is free software under the GNU General Public License version 3 or
later (LICENSE.txt). It contains portions of ScummVM (https://www.scummvm.org/),
also licensed GPL-3.0-or-later.

The statically linked Nuked OPL3 core is LGPL-2.1-or-later
(licenses\Nuked-LGPL.txt), copyright 2013-2020 Nuke.YKT and 2026 Tony Gies.
It is the Nuked-OPL3-fast 1.8-fast.2 core from the pinned ScummVM source below.

Corresponding source:
  MMModern  https://github.com/carbiecarbie/MMModern  tag v$version
  ScummVM   https://github.com/scummvm/scummvm.git    revision $scummvm

Bundled runtime libraries, MSYS2 UCRT64 builds (license texts in licenses\):
  SDL2.dll             Simple DirectMedia Layer, zlib license (licenses\SDL2.txt)
  zlib1.dll            zlib, zlib license (licenses\zlib.txt)
  libwinpthread-1.dll  mingw-w64 winpthreads, MIT-style license (licenses\winpthreads.txt)

The GCC runtime (libgcc, libstdc++) is linked statically under the GPL version 3
with the GCC Runtime Library Exception, version 3.1
(licenses\GCC-runtime-exception.txt).

Package versions:
  $packages

No Might and Magic game data is included. The original game is required.
"@ | Set-Content -Encoding ascii (Join-Path $stage 'THIRD-PARTY.txt')
# The launcher finds and remembers the game folder; the batch files only start
# it. Windows batch files need CRLF line endings.
Copy-Item (Join-Path $PSScriptRoot 'release\MMModern-launcher.ps1') $stage
function Write-Batch([string]$file, [string]$arguments) {
	$text = "@echo off`r`npowershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File `"%~dp0MMModern-launcher.ps1`"$arguments`r`nif errorlevel 1 pause`r`n"
	[IO.File]::WriteAllText((Join-Path $stage $file), $text, [Text.Encoding]::ASCII)
}
Write-Batch 'MMModern.bat' ''
Write-Batch 'Change game folder.bat' ' -Change'

# Name entries explicitly: PowerShell 5.1's archive APIs write '\' separators.
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
try {
	foreach ($file in Get-ChildItem $stage -Recurse -File) {
		$entry = "$name/" + $file.FullName.Substring($stage.Length + 1).Replace('\', '/')
		[void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entry, [IO.Compression.CompressionLevel]::Optimal)
	}
} finally { $archive.Dispose() }
Get-ChildItem $stage | Select-Object Name, Length | Format-Table -AutoSize | Out-String | Write-Output
Write-Output "Package: $zip ($((Get-Item $zip).Length) bytes)"
