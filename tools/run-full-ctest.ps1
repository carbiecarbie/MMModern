# Runs the complete CTest suite for an already configured and built directory,
# writes the full output to a log and publishes a short summary only when the
# run has finished. Intended for the maintainer's terminal or a lightweight
# runner, so an agent does not have to monitor a long run.
#
# Example:
#   $env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
#   .\tools\run-full-ctest.ps1 -BuildDir build-rel
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDir,
    [string]$CTest = 'ctest',
    [ValidateRange(1, 256)][int]$Jobs = 1,
    [string]$Configuration = ''
)

$ErrorActionPreference = 'Stop'
$build = (Resolve-Path -LiteralPath $BuildDir).Path
$ctestExe = (Get-Command $CTest -CommandType Application | Select-Object -First 1).Source

if (-not (Test-Path -LiteralPath (Join-Path $build 'CTestTestfile.cmake'))) {
    throw "No CTest configuration in $build"
}

$runId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' +
    [guid]::NewGuid().ToString('N').Substring(0, 8)
$resultDir = Join-Path $build "ctest-results\$runId"
$null = New-Item -ItemType Directory -Path $resultDir
$log = Join-Path $resultDir 'full.log'
$summary = Join-Path $resultDir 'summary.txt'

$ctestArgs = @(
    '--test-dir', $build, '--no-tests=error',
    '--output-on-failure', '--parallel', "$Jobs"
)
if ($Configuration) {
    $ctestArgs += @('--build-config', $Configuration)
}

Write-Host "Running full suite. Results: $resultDir"
$started = [datetimeoffset]::Now
$timer = [Diagnostics.Stopwatch]::StartNew()
$code = 1
$runnerError = ''

try {
    # Windows PowerShell 5.1 turns redirected native stderr into error records;
    # do not let them abort the run.
    $ErrorActionPreference = 'Continue'
    & $ctestExe @ctestArgs *> $log
    $code = $LASTEXITCODE
}
catch {
    $runnerError = $_.Exception.Message
}
finally {
    $ErrorActionPreference = 'Stop'
    $timer.Stop()
    $highlights = @()
    if (Test-Path -LiteralPath $log) {
        $highlights = @(
            Select-String -LiteralPath $log -Pattern (
                '^\d+% tests passed|^Total Test time|' +
                '^\s*\d+\s+-\s+|No tests were found|CMake Error|CTest Error'
            ) | ForEach-Object { $_.Line }
        )
    }

    @(
        "Started: $($started.ToString('o'))"
        "Finished: $([datetimeoffset]::Now.ToString('o'))"
        "Executable: $ctestExe"
        "Arguments: $($ctestArgs -join ' ')"
        "Build directory: $build"
        "Exit code: $code"
        "Duration seconds: $([math]::Round($timer.Elapsed.TotalSeconds, 1))"
        "Runner error: $runnerError"
        "Full output and failure details: $log"
        ''
        $highlights
    ) | Set-Content -LiteralPath "$summary.tmp" -Encoding utf8

    Move-Item -LiteralPath "$summary.tmp" -Destination $summary
    Write-Host "Finished. Exit code: $code. Summary: $summary"
}

exit $code
