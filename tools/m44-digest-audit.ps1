# Requires Python 3 and the configured UCRT64 toolchain (objdump) on PATH.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Preserve','Compare')][string]$Step,
    [Parameter(Mandatory)][string]$BuildDir,
    [Parameter(Mandatory)][string]$PreservedDir,
    [Parameter(Mandatory)][string]$GameDir,
    [string]$OutputDir = '',
    [string]$Python = 'python',
    [string]$Objdump = 'objdump'
)
$ErrorActionPreference = 'Stop'
$arguments = @((Join-Path $PSScriptRoot 'm44_digest_audit.py'), $Step,
    '--build-dir', $BuildDir, '--preserved-dir', $PreservedDir,
    '--game-dir', $GameDir, '--objdump', $Objdump)
if ($OutputDir) { $arguments += @('--output-dir', $OutputDir) }
& $Python @arguments
exit $LASTEXITCODE
