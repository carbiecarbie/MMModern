param(
	[Parameter(Mandatory = $true)][string]$Generator,
	[Parameter(Mandatory = $true)][string]$TestPowerShell,
	[Parameter(Mandatory = $true)][string]$Git,
	[Parameter(Mandatory = $true)][string]$TestRoot,
	[Parameter(Mandatory = $true)][string]$ProductionOutput,
	[Parameter(Mandatory = $true)][string]$ProductionOutputSha256
)

$ErrorActionPreference = 'Stop'
$CatalogStart = 20680
$CatalogEnd = 22438
$CatalogSize = 35065
$BlobPath = 'devtools/create_mm/files/xeen/CONSTANTS_7'
$Counts = @(7, 41, 14, 11, 22, 74)

function Check([bool]$Condition, [string]$Message) {
	if (-not $Condition) { throw $Message }
}

function Get-Sha256([byte[]]$Bytes) {
	$algorithm = [Security.Cryptography.SHA256]::Create()
	try {
		return [BitConverter]::ToString($algorithm.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant()
	} finally {
		$algorithm.Dispose()
	}
}

function Run-Git([string]$Repository, [string[]]$CommandArguments) {
	$output = & $Git -C $Repository @CommandArguments 2>&1
	if ($LASTEXITCODE -ne 0) { throw "Fixture Git failed: $output" }
	return ($output -join "`n").Trim()
}

function Add-Token([Collections.Generic.List[byte]]$Bytes, [byte[]]$Token) {
	$Bytes.AddRange($Token)
	$Bytes.Add(0)
}

function New-CatalogBlob([string]$Mode = 'valid') {
	$ascii = [Text.Encoding]::ASCII
	$scalars = New-Object System.Collections.ArrayList
	$broken = $ascii.GetBytes(([string][char]12) + '32broken ')
	if ($Mode -eq 'oversized') { $broken = $ascii.GetBytes(('x' * 64)) }
	[void]$scalars.Add($broken)
	[void]$scalars.Add($ascii.GetBytes(([string][char]12) + '09cursed '))
	[void]$scalars.Add($ascii.GetBytes('of '))

	$tables = New-Object System.Collections.ArrayList
	for ($table = 0; $table -lt $Counts.Count; ++$table) {
		$entries = New-Object System.Collections.ArrayList
		for ($entry = 0; $entry -lt $Counts[$table]; ++$entry) {
			$value = if ($entry -eq 0) { '' } else { "t$table-$entry" }
			[void]$entries.Add($ascii.GetBytes($value))
		}
		[void]$tables.Add($entries)
	}
	$tables[0][1] = $ascii.GetBytes('Dragon Slayer')
	$tables[1][12] = $ascii.GetBytes('dagger ')
	$tables[1][40] = $ascii.GetBytes('Elder LongBow ')
	$tables[2][10] = $ascii.GetBytes('boots ')
	$tables[3][1] = $ascii.GetBytes('ring ')
	$tables[4][10] = $ascii.GetBytes('potion ')
	$tables[5][37] = $ascii.GetBytes('antidotes')
	$tables[5][73] = $ascii.GetBytes('the GODS!')

	$length = 24
	foreach ($scalar in $scalars) { $length += $scalar.Length + 1 }
	foreach ($entries in $tables) {
		foreach ($entry in $entries) { $length += $entry.Length + 1 }
	}
	$remaining = ($CatalogEnd - $CatalogStart) - $length
	$protected = @('0:1', '1:12', '1:40', '2:10', '3:1', '4:10', '5:37', '5:73')
	for ($table = 0; $table -lt $tables.Count -and $remaining -gt 0; ++$table) {
		for ($entry = 1; $entry -lt $tables[$table].Count -and $remaining -gt 0; ++$entry) {
			if ($protected -contains "$table`:$entry") { continue }
			$old = [byte[]]$tables[$table][$entry]
			$add = [Math]::Min(63 - $old.Length, $remaining)
			if ($add -le 0) { continue }
			$padded = New-Object byte[] ($old.Length + $add)
			[Array]::Copy($old, $padded, $old.Length)
			for ($index = $old.Length; $index -lt $padded.Length; ++$index) {
				$padded[$index] = [byte][char]'x'
			}
			$tables[$table][$entry] = $padded
			$remaining -= $add
		}
	}
	Check ($remaining -eq 0) 'Synthetic catalog block could not reach the fixed extent'

	$block = New-Object 'System.Collections.Generic.List[byte]'
	foreach ($scalar in $scalars) { Add-Token $block $scalar }
	for ($table = 0; $table -lt $tables.Count; ++$table) {
		$block.Add(0); $block.Add(0); $block.Add(0); $block.Add([byte]$Counts[$table])
		foreach ($entry in $tables[$table]) { Add-Token $block $entry }
	}
	Check ($block.Count -eq ($CatalogEnd - $CatalogStart)) 'Synthetic catalog block size differs'
	if ($Mode -eq 'bad-count') {
		$firstTag = 0
		foreach ($scalar in $scalars) { $firstTag += $scalar.Length + 1 }
		$block[$firstTag + 3] = 8
	}
	$blob = New-Object byte[] $CatalogSize
	[Array]::Copy($block.ToArray(), 0, $blob, $CatalogStart, $block.Count)
	if ($Mode -eq 'truncated') {
		$short = New-Object byte[] ($CatalogSize - 1)
		[Array]::Copy($blob, $short, $short.Length)
		return ,$short
	}
	return ,$blob
}

function New-TestRepository([string]$Name, [byte[]]$Blob) {
	$source = Join-Path $TestRoot $Name
	$input = Join-Path $source $BlobPath
	[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($input)) | Out-Null
	[IO.File]::WriteAllBytes($input, $Blob)
	Run-Git $source @('init', '-q') | Out-Null
	Run-Git $source @('config', 'user.name', 'MMModern test') | Out-Null
	Run-Git $source @('config', 'user.email', 'test@example.invalid') | Out-Null
	Run-Git $source @('add', '.') | Out-Null
	Run-Git $source @('commit', '-m', 'catalog fixture') | Out-Null
	$revision = Run-Git $source @('rev-parse', 'HEAD')
	$oid = Run-Git $source @('hash-object', '--', $BlobPath)
	return [PSCustomObject]@{
		Source = $source
		Input = $input
		Revision = $revision
		Oid = $oid
		Sha256 = Get-Sha256 $Blob
	}
}

function Invoke-Generator($Repository, [string]$Output, [bool]$ExpectedSuccess,
		[string]$Revision = '', [string]$Oid = '', [string]$Sha256 = '') {
	if (-not $Revision) { $Revision = $Repository.Revision }
	if (-not $Oid) { $Oid = $Repository.Oid }
	if (-not $Sha256) { $Sha256 = $Repository.Sha256 }
	$arguments = @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
		'-File', $Generator, '-Source', $Repository.Source, '-Revision', $Revision,
		'-Git', $Git, '-ExpectedBlobOid', $Oid, '-ExpectedBlobSha256', $Sha256,
		'-Output', $Output)
	$oldErrorActionPreference = $ErrorActionPreference
	try {
		$ErrorActionPreference = 'Continue'
		$result = & $TestPowerShell @arguments 2>&1
		$exitCode = $LASTEXITCODE
	} finally {
		$ErrorActionPreference = $oldErrorActionPreference
	}
	$success = $exitCode -eq 0
	if ($success -ne $ExpectedSuccess) {
		throw "Generator exit differed (expected $ExpectedSuccess): $result"
	}
	if (-not $success -and -not (($result -join "`n").Contains('Item catalog generation failed:'))) {
		throw "Generator failure did not report its error: $result"
	}
}

function ConvertTo-OctalLiteral([string]$Value) {
	$builder = New-Object Text.StringBuilder
	[void]$builder.Append('"')
	foreach ($byte in [Text.Encoding]::ASCII.GetBytes($Value)) {
		[void]$builder.Append(('\{0}{1}{2}' -f
			(($byte -shr 6) -band 7), (($byte -shr 3) -band 7), ($byte -band 7)))
	}
	[void]$builder.Append('"')
	return $builder.ToString()
}

try {
	$TestRoot = Join-Path $TestRoot ('run-' + [Guid]::NewGuid().ToString('N'))
	[IO.Directory]::CreateDirectory($TestRoot) | Out-Null
	$valid = New-TestRepository 'valid' (New-CatalogBlob)
	$output = Join-Path $TestRoot 'generated/catalog.inc'
	Check (-not [IO.File]::Exists($output)) 'Initial-publication destination unexpectedly exists'
	Invoke-Generator $valid $output $true
	Check ([IO.File]::Exists($output)) 'Initial publication did not create output'
	$text = [IO.File]::ReadAllText($output)
	Check $text.Contains('inline constexpr unsigned kSchema = 1;') 'Generated schema differs'
	Check $text.Contains('inline constexpr unsigned kLanguage = 7;') 'Generated language differs'
	$names = @('BonusNames', 'WeaponNames', 'ArmorNames', 'AccessoryNames', 'MiscNames', 'SpecialNames')
	for ($index = 0; $index -lt $Counts.Count; ++$index) {
		Check ($text.Contains("std::array<std::string_view, $($Counts[$index])> k$($names[$index])")) `
			"Generated count differs: $($names[$index])"
	}
	$literals = @(
		(([string][char]12) + '32broken ')
		(([string][char]12) + '09cursed ')
		'of '
		'Dragon Slayer'
		'dagger '
		'boots '
		'ring '
		'potion '
		'antidotes'
		'the GODS!'
	)
	foreach ($literal in $literals) {
		Check $text.Contains((ConvertTo-OctalLiteral $literal)) "Generated literal differs: $literal"
	}
	$firstBytes = [IO.File]::ReadAllBytes($output)
	$timestamp = [IO.File]::GetLastWriteTimeUtc($output)
	Start-Sleep -Milliseconds 1100
	Invoke-Generator $valid $output $true
	Check (Test-Path -LiteralPath $output) 'Deterministic regeneration removed output'
	Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq (Get-Sha256 $firstBytes)) `
		'Deterministic regeneration changed bytes'
	Check ([IO.File]::GetLastWriteTimeUtc($output) -eq $timestamp) `
		'Unchanged generation replaced output'

	# An existing, different but valid C++ file must be atomically replaced. This
	# path also verifies argument and publication handling when directories and the
	# destination filename contain spaces.
	$replacementOutput = Join-Path $TestRoot 'replacement path with spaces/generated catalog.inc'
	[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($replacementOutput)) | Out-Null
	$oldReplacement = "namespace previous_valid_output { inline constexpr int value = 1; }`n"
	[IO.File]::WriteAllText($replacementOutput, $oldReplacement)
	Invoke-Generator $valid $replacementOutput $true
	$replacementBytes = [IO.File]::ReadAllBytes($replacementOutput)
	Check ($replacementBytes.Length -eq $firstBytes.Length) `
		'Changed-output replacement length differs'
	Check ((Get-Sha256 $replacementBytes) -eq (Get-Sha256 $firstBytes)) `
		'Changed-output replacement bytes differ'
	Check (-not [IO.File]::ReadAllText($replacementOutput).Contains('previous_valid_output')) `
		'Changed-output replacement retained old contents'
	$replacementTemps = @(Get-ChildItem -LiteralPath ([IO.Path]::GetDirectoryName($replacementOutput)) `
		-Filter ([IO.Path]::GetFileName($replacementOutput) + '.tmp-*') -File)
	Check ($replacementTemps.Count -eq 0) 'Changed-output replacement left a temporary file'

	# Hold an existing destination without delete sharing. Windows File.Replace
	# must fail while leaving that destination intact, and the child generator's
	# finally block must remove its sibling temporary file.
	$failureOutput = Join-Path $TestRoot 'replacement-failure/catalog.inc'
	[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($failureOutput)) | Out-Null
	$failureSentinel = "namespace preserved_valid_output { inline constexpr int value = 2; }`n"
	[IO.File]::WriteAllText($failureOutput, $failureSentinel)
	$destinationLock = [IO.File]::Open($failureOutput, [IO.FileMode]::Open,
		[IO.FileAccess]::Read, [IO.FileShare]::Read)
	try {
		Invoke-Generator $valid $failureOutput $false
	} finally {
		$destinationLock.Dispose()
	}
	Check ([IO.File]::ReadAllText($failureOutput) -eq $failureSentinel) `
		'Replacement failure changed the previous destination'
	$failureTemps = @(Get-ChildItem -LiteralPath ([IO.Path]::GetDirectoryName($failureOutput)) `
		-Filter ([IO.Path]::GetFileName($failureOutput) + '.tmp-*') -File)
	Check ($failureTemps.Count -eq 0) 'Replacement failure left a temporary file'

	# Mutable worktree/compiler artifacts are outside the generation path. The
	# same pinned object must still produce the identical include.
	[IO.File]::WriteAllBytes($valid.Input, (New-Object byte[] $CatalogSize))
	[IO.File]::WriteAllText($valid.Input + '.gch', 'untracked PCH')
	[IO.File]::WriteAllText((Join-Path $valid.Source 'forced.h'), '#error forced input')
	[IO.File]::WriteAllText((Join-Path $valid.Source 'options.rsp'), '--include=forced.h')
	$oldCxxFlags = $env:CXXFLAGS
	$oldIncludePath = $env:CPLUS_INCLUDE_PATH
	try {
		$env:CXXFLAGS = '--include=forced.h @options.rsp'
		$env:CPLUS_INCLUDE_PATH = $valid.Source
		Invoke-Generator $valid $output $true
	} finally {
		$env:CXXFLAGS = $oldCxxFlags
		$env:CPLUS_INCLUDE_PATH = $oldIncludePath
	}
	Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq (Get-Sha256 $firstBytes)) `
		'Mutable worktree/compiler artifacts influenced the pinned blob output'

	$preserved = [IO.File]::ReadAllBytes($output)
	Invoke-Generator $valid $output $false ('0' * 40)
	Invoke-Generator $valid $output $false $valid.Revision ('0' * 40)
	Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq (Get-Sha256 $preserved)) `
		'Identity failure changed existing output'
	Run-Git $valid.Source @('add', $BlobPath) | Out-Null
	Run-Git $valid.Source @('commit', '-m', 'changed catalog input') | Out-Null
	Invoke-Generator $valid $output $false

	foreach ($mode in @('bad-count', 'oversized', 'truncated')) {
		$invalid = New-TestRepository $mode (New-CatalogBlob $mode)
		$sentinel = Join-Path $TestRoot "$mode.inc"
		[IO.File]::WriteAllText($sentinel, 'last valid include')
		Invoke-Generator $invalid $sentinel $false
		Check ([IO.File]::ReadAllText($sentinel) -eq 'last valid include') `
			"$mode input partially published output"
	}

	Check ([IO.File]::Exists($ProductionOutput)) 'Production generated include is missing'
	Check ((Get-Sha256 ([IO.File]::ReadAllBytes($ProductionOutput))) -eq
		$ProductionOutputSha256.ToLowerInvariant()) 'Production generated include identity differs'
	Write-Output 'Pinned-blob item catalog generation tests passed'
} catch {
	[Console]::Error.WriteLine($_.Exception.Message)
	exit 1
}
