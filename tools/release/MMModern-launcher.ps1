# MMModern release launcher. Finds the World of Xeen folder (GOG registry
# entries first, then a folder picker), remembers it per user and starts the
# game. Ships in the release zip; keep this file ASCII for Windows PowerShell 5.1.
param([switch]$Change)
$ErrorActionPreference = 'Stop'
$settings = Join-Path $env:LOCALAPPDATA 'MMModern'
$remembered = Join-Path $settings 'game-folder.txt'

function Test-GameFolder([string]$folder) {
	return [bool]$folder -and (Test-Path -LiteralPath (Join-Path $folder 'WORLD\XEEN.DAT') -PathType Leaf)
}

# Accept the game folder itself, or its WORLD subfolder picked by mistake.
function Resolve-GameFolder([string]$folder) {
	if (Test-GameFolder $folder) { return $folder }
	$parent = Split-Path -LiteralPath $folder -Parent
	if ($parent -and (Split-Path -Leaf $folder) -eq 'WORLD' -and (Test-GameFolder $parent)) { return $parent }
	return $null
}

function Find-GogFolder {
	foreach ($root in 'HKLM:\SOFTWARE\WOW6432Node\GOG.com\Games', 'HKLM:\SOFTWARE\GOG.com\Games', 'HKCU:\SOFTWARE\GOG.com\Games') {
		if (-not (Test-Path $root)) { continue }
		foreach ($key in Get-ChildItem $root -ErrorAction SilentlyContinue) {
			$entry = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
			if ($entry -and $entry.path -and (Test-GameFolder $entry.path)) { return $entry.path }
		}
	}
	return $null
}

function Select-GameFolder {
	Add-Type -AssemblyName System.Windows.Forms
	# A topmost owner keeps the dialogs in front of the console window.
	$owner = New-Object System.Windows.Forms.Form -Property @{ TopMost = $true; ShowInTaskbar = $false }
	try {
		while ($true) {
			$dialog = New-Object System.Windows.Forms.FolderBrowserDialog
			$dialog.Description = 'Choose your "Might and Magic 4-5" folder from GOG (the folder that contains WORLD\XEEN.DAT).'
			$dialog.ShowNewFolderButton = $false
			if ($dialog.ShowDialog($owner) -ne [System.Windows.Forms.DialogResult]::OK) { return $null }
			$folder = Resolve-GameFolder $dialog.SelectedPath
			if ($folder) { return $folder }
			[void][System.Windows.Forms.MessageBox]::Show($owner,
				"This folder does not contain WORLD\XEEN.DAT:`n`n$($dialog.SelectedPath)`n`nChoose the Might and Magic 4-5 folder installed by GOG.",
				'MMModern', [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Warning)
		}
	} finally { $owner.Dispose() }
}

$folder = $null
if (-not $Change -and (Test-Path -LiteralPath $remembered -PathType Leaf)) {
	$saved = (Get-Content -LiteralPath $remembered -Encoding UTF8 -TotalCount 1)
	if ($saved -and (Test-GameFolder $saved.Trim())) { $folder = $saved.Trim() }
}
if (-not $folder -and -not $Change) { $folder = Find-GogFolder }
if (-not $folder) { $folder = Select-GameFolder }
if (-not $folder) {
	Write-Output 'No game folder chosen; MMModern was not started.'
	exit 0
}
New-Item -ItemType Directory -Force -Path $settings | Out-Null
Set-Content -LiteralPath $remembered -Value $folder -Encoding UTF8

Write-Output "Game folder: $folder"
& (Join-Path $PSScriptRoot 'mmodern.exe') $folder
exit $LASTEXITCODE
