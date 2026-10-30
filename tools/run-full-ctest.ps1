# Runs the complete CTest suite for an already configured and built directory,
# writes the full output to a log and publishes a short summary only when the
# run has finished. Intended for the maintainer's terminal or a lightweight
# runner, so an agent does not have to monitor a long run.
#
# Parallel jobs: explicit -Jobs, then this computer's local preference, then
# half the logical processors (at most 8). See tools/AGENTS.md for configuration.
# CMake marks the real-clock and native-window tests RUN_SERIAL and locks the
# tests that share a fixed path, so the jobs never interfere; every timeout is
# a hang guard independent of load (CMakeLists.txt, test policy block).
#
# The script refuses to run inside an agent sandbox and puts the configured
# compiler's directory on PATH itself.
#
# Example:
#   .\tools\run-full-ctest.ps1 -BuildDir build-rel
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDir,
    [string]$CTest = 'ctest',
    [ValidateRange(0, 256)][int]$Jobs = 0,
    [string]$Configuration = ''
)

$ErrorActionPreference = 'Stop'
$build = (Resolve-Path -LiteralPath $BuildDir).Path

if (-not (Test-Path -LiteralPath (Join-Path $build 'CTestTestfile.cmake'))) {
    throw "No CTest configuration in $build"
}

# A sandboxed run is invalid, not a test result (tools/AGENTS.md): refuse it
# before spending minutes on failures it would cause.
if ("$env:TEMP" -match '\\AppData\\Local\\Packages\\') {
    throw "TEMP points into an app sandbox ($env:TEMP); run the complete suite unsandboxed"
}

if ($Jobs -eq 0 -and $env:LOCALAPPDATA) {
    $jobsFile = Join-Path $env:LOCALAPPDATA 'MMModern\ctest-jobs.json'
    if (Test-Path -LiteralPath $jobsFile) {
        try {
            $preference = Get-Content -LiteralPath $jobsFile -Raw | ConvertFrom-Json
        }
        catch {
            throw "Cannot read CTest job preference ${jobsFile}: $($_.Exception.Message)"
        }
        if ($preference.computerName -eq [Environment]::MachineName) {
            $preferredJobs = 0
            if (-not [int]::TryParse([string]$preference.jobs, [ref]$preferredJobs) -or
                $preferredJobs -lt 1 -or $preferredJobs -gt 256) {
                throw "CTest job preference $jobsFile must specify an integer jobs value from 1 to 256"
            }
            $Jobs = $preferredJobs
        }
    }
}
if ($Jobs -eq 0) {
    $Jobs = [math]::Max(1, [math]::Min(8, [int][math]::Floor([Environment]::ProcessorCount / 2)))
}

# Test executables need the toolchain's runtime DLLs; without its directory on
# PATH they fail with 0xc0000139. Prepend the configured compiler's directory.
$compilerLine = Select-String -LiteralPath (Join-Path $build 'CMakeCache.txt') `
    -Pattern '^CMAKE_CXX_COMPILER:\w+=(.+)$' | Select-Object -First 1
if ($compilerLine) {
    $toolchainBin = Split-Path -Parent $compilerLine.Matches[0].Groups[1].Value
    if (Test-Path -LiteralPath $toolchainBin) {
        $env:PATH = "$toolchainBin;$env:PATH"
    }
}
$ctestExe = (Get-Command $CTest -CommandType Application | Select-Object -First 1).Source

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

Write-Host "Running full suite with $Jobs parallel jobs. Results: $resultDir"
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
        "Parallel jobs: $Jobs"
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
