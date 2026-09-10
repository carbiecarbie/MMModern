param(
	[Parameter(Mandatory = $true)][string]$Source,
	[Parameter(Mandatory = $true)][string]$Revision,
	[Parameter(Mandatory = $true)][string]$Git,
	[Parameter(Mandatory = $true)][string]$ExpectedBlobOid,
	[Parameter(Mandatory = $true)][string]$ExpectedBlobSha256,
	[Parameter(Mandatory = $true)][string]$Output
)

$ErrorActionPreference = 'Stop'
$CatalogBlobPath = 'devtools/create_mm/files/xeen/CONSTANTS_7'
$CatalogBlobSize = 35065
$CatalogStart = 20680
$CatalogEnd = 22438
$TokenLimit = 63
$OutputLimit = 16384
$TableNames = @('BonusNames', 'WeaponNames', 'ArmorNames', 'AccessoryNames', 'MiscNames', 'SpecialNames')
$TableCounts = @(7, 41, 14, 11, 22, 74)

function Quote-ProcessArgument([string]$Value) {
	if ($Value.Contains('"') -or $Value.EndsWith('\')) {
		throw 'Unsupported Git argument'
	}
	return '"' + $Value + '"'
}

function New-GitProcess([string]$Arguments, [string]$GitDirectory = '') {
	$info = New-Object System.Diagnostics.ProcessStartInfo
	$info.FileName = $Git
	$info.UseShellExecute = $false
	$info.CreateNoWindow = $true
	$info.RedirectStandardOutput = $true
	$info.RedirectStandardError = $true
	$info.WorkingDirectory = $script:SourcePath
	foreach ($key in @($info.EnvironmentVariables.Keys)) {
		if ($null -ne $key -and $key.StartsWith('GIT_', [StringComparison]::OrdinalIgnoreCase)) {
			$info.EnvironmentVariables.Remove($key)
		}
	}
	$info.EnvironmentVariables['GIT_CONFIG_NOSYSTEM'] = '1'
	$info.EnvironmentVariables['GIT_CONFIG_GLOBAL'] = 'NUL'
	$info.EnvironmentVariables['GIT_NO_REPLACE_OBJECTS'] = '1'
	$prefix = '--no-replace-objects -c ' +
		(Quote-ProcessArgument ('safe.directory=' + $script:SourcePath)) + ' '
	if ($GitDirectory) {
		$prefix += '--git-dir=' + (Quote-ProcessArgument $GitDirectory) + ' '
	}
	$info.Arguments = $prefix + $Arguments
	return New-Object System.Diagnostics.Process -Property @{ StartInfo = $info }
}

function Read-GitText([string]$Arguments, [string]$GitDirectory = '') {
	$process = New-GitProcess $Arguments $GitDirectory
	[void]$process.Start()
	$errorTask = $process.StandardError.ReadToEndAsync()
	$text = $process.StandardOutput.ReadToEnd()
	$process.WaitForExit()
	if ($process.ExitCode -ne 0) {
		throw 'Git plumbing failed: ' + $errorTask.Result
	}
	$process.Dispose()
	return $text
}

function Read-GitBytes([string]$Arguments, [string]$GitDirectory) {
	$process = New-GitProcess $Arguments $GitDirectory
	[void]$process.Start()
	$errorTask = $process.StandardError.ReadToEndAsync()
	$memory = New-Object System.IO.MemoryStream
	$process.StandardOutput.BaseStream.CopyTo($memory)
	$process.WaitForExit()
	if ($process.ExitCode -ne 0) {
		throw 'Git blob read failed: ' + $errorTask.Result
	}
	$bytes = $memory.ToArray()
	$memory.Dispose()
	$process.Dispose()
	return ,([byte[]]$bytes)
}

function Get-Sha256([byte[]]$Bytes) {
	$algorithm = [Security.Cryptography.SHA256]::Create()
	try {
		return [BitConverter]::ToString($algorithm.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant()
	} finally {
		$algorithm.Dispose()
	}
}

function Read-CatalogToken([byte[]]$Bytes, [ref]$Position, [string]$Label) {
	$start = $Position.Value
	while ($Position.Value -lt $script:CatalogEnd -and $Bytes[$Position.Value] -ne 0) {
		$Position.Value++
	}
	if ($Position.Value -ge $script:CatalogEnd) {
		throw "Truncated catalog token: $Label"
	}
	$length = $Position.Value - $start
	if ($length -gt $script:TokenLimit) {
		throw "Catalog token exceeds 63 bytes: $Label"
	}
	$token = New-Object byte[] $length
	if ($length -ne 0) {
		[Array]::Copy($Bytes, $start, $token, 0, $length)
	}
	$Position.Value++
	return ,$token
}

function Read-Catalog([byte[]]$Bytes) {
	$position = $script:CatalogStart
	$scalars = New-Object System.Collections.ArrayList
	foreach ($name in @('ITEM_BROKEN', 'ITEM_CURSED', 'ITEM_OF')) {
		$token = Read-CatalogToken $Bytes ([ref]$position) $name
		if ($token.Length -eq 0) {
			throw "Catalog scalar is empty: $name"
		}
		[void]$scalars.Add($token)
	}
	$tables = New-Object System.Collections.ArrayList
	for ($tableIndex = 0; $tableIndex -lt $script:TableCounts.Count; ++$tableIndex) {
		$count = $script:TableCounts[$tableIndex]
		if ($position + 4 -gt $script:CatalogEnd -or
				$Bytes[$position] -ne 0 -or $Bytes[$position + 1] -ne 0 -or
				$Bytes[$position + 2] -ne 0 -or $Bytes[$position + 3] -ne $count) {
			throw "Catalog array count tag mismatch: $($script:TableNames[$tableIndex])"
		}
		$position += 4
		$entries = New-Object System.Collections.ArrayList
		for ($entryIndex = 0; $entryIndex -lt $count; ++$entryIndex) {
			$token = Read-CatalogToken $Bytes ([ref]$position) `
				("$($script:TableNames[$tableIndex])[$entryIndex]")
			if (($entryIndex -eq 0 -and $token.Length -ne 0) -or
					($entryIndex -ne 0 -and $token.Length -eq 0)) {
				throw "Catalog reserved/required entry mismatch: $($script:TableNames[$tableIndex])[$entryIndex]"
			}
			[void]$entries.Add($token)
		}
		[void]$tables.Add([PSCustomObject]@{
			Name = $script:TableNames[$tableIndex]
			Entries = $entries
		})
	}
	if ($position -ne $script:CatalogEnd) {
		throw 'Catalog block end offset differs'
	}
	return [PSCustomObject]@{ Scalars = $scalars; Tables = $tables }
}

function ConvertTo-OctalLiteral([byte[]]$Bytes) {
	$builder = New-Object Text.StringBuilder
	[void]$builder.Append('"')
	foreach ($byte in $Bytes) {
		[void]$builder.Append('\')
		[void]$builder.Append([char]([int][char]'0' + (($byte -shr 6) -band 7)))
		[void]$builder.Append([char]([int][char]'0' + (($byte -shr 3) -band 7)))
		[void]$builder.Append([char]([int][char]'0' + ($byte -band 7)))
	}
	[void]$builder.Append('"')
	return $builder.ToString()
}

function Add-Line([Text.StringBuilder]$Builder, [string]$Line) {
	[void]$Builder.Append($Line)
	[void]$Builder.Append("`n")
}

function New-GeneratedInclude($Catalog, [string]$SourceRevision) {
	$builder = New-Object Text.StringBuilder
	Add-Line $builder "/* Generated from ScummVM $SourceRevision; GPL-3.0-or-later; see docs/dependencies.md and ScummVM COPYRIGHT. */"
	Add-Line $builder 'namespace mmodern::generated_item_catalog {'
	Add-Line $builder 'inline constexpr unsigned kSchema = 1;'
	Add-Line $builder 'inline constexpr unsigned kLanguage = 7;'
	$revisionBytes = [Text.Encoding]::ASCII.GetBytes($SourceRevision)
	Add-Line $builder ('inline constexpr std::string_view kSourceRevision = ' +
		(ConvertTo-OctalLiteral $revisionBytes) + ';')
	$scalarNames = @('ItemBroken', 'ItemCursed', 'ItemOf')
	for ($index = 0; $index -lt $scalarNames.Count; ++$index) {
		Add-Line $builder ('inline constexpr std::string_view k' + $scalarNames[$index] + ' = ' +
			(ConvertTo-OctalLiteral $Catalog.Scalars[$index]) + ';')
	}
	foreach ($table in $Catalog.Tables) {
		Add-Line $builder ("inline constexpr std::array<std::string_view, $($table.Entries.Count)> k$($table.Name){{")
		foreach ($entry in $table.Entries) {
			Add-Line $builder ("`t" + (ConvertTo-OctalLiteral $entry) + ',')
		}
		Add-Line $builder '}};'
	}
	Add-Line $builder '} // namespace mmodern::generated_item_catalog'
	$result = $builder.ToString()
	if ([Text.Encoding]::UTF8.GetByteCount($result) -gt $script:OutputLimit) {
		throw 'Generated include exceeds 16,384 bytes'
	}
	return $result
}

function Test-BytesEqual([byte[]]$First, [byte[]]$Second) {
	if ($First.Length -ne $Second.Length) { return $false }
	for ($index = 0; $index -lt $First.Length; ++$index) {
		if ($First[$index] -ne $Second[$index]) { return $false }
	}
	return $true
}

function Publish-Atomically([string]$Path, [string]$Contents) {
	$fullPath = [IO.Path]::GetFullPath($Path)
	$directory = [IO.Path]::GetDirectoryName($fullPath)
	[IO.Directory]::CreateDirectory($directory) | Out-Null
	$encoding = New-Object Text.UTF8Encoding $false
	$bytes = $encoding.GetBytes($Contents)
	if ([IO.File]::Exists($fullPath) -and
			(Test-BytesEqual ([IO.File]::ReadAllBytes($fullPath)) $bytes)) {
		return
	}
	$temporary = $fullPath + '.tmp-' + $PID + '-' + [Guid]::NewGuid().ToString('N')
	try {
		[IO.File]::WriteAllBytes($temporary, $bytes)
		$stream = [IO.File]::Open($temporary, [IO.FileMode]::Open,
			[IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
		try { $stream.Flush($true) } finally { $stream.Dispose() }
		if ([IO.File]::Exists($fullPath)) {
			[IO.File]::Replace($temporary, $fullPath,
				[System.Management.Automation.Language.NullString]::Value, $true)
		} else {
			[IO.File]::Move($temporary, $fullPath)
		}
	} finally {
		if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) }
	}
}

try {
	$SourcePath = [IO.Path]::GetFullPath($Source).TrimEnd('\', '/')
	if (-not [IO.Directory]::Exists($SourcePath) -or
			(-not [IO.Directory]::Exists((Join-Path $SourcePath '.git')) -and
			 -not [IO.File]::Exists((Join-Path $SourcePath '.git')))) {
		throw "ScummVM source is not a Git checkout: $SourcePath"
	}
	if ($Revision -notmatch '^[0-9a-f]{40}$' -or $ExpectedBlobOid -notmatch '^[0-9a-f]{40}$' -or
			$ExpectedBlobSha256 -notmatch '^[0-9a-f]{64}$') {
		throw 'Invalid pinned catalog identity'
	}
	$gitDirectory = (Read-GitText 'rev-parse --absolute-git-dir').Trim()
	$head = (Read-GitText 'rev-parse --verify HEAD' $gitDirectory).Trim()
	if ($head -ne $Revision) { throw 'ScummVM catalog source revision mismatch' }
	$treeEntry = (Read-GitText ("ls-tree $Revision -- $CatalogBlobPath") $gitDirectory).Trim()
	$expectedEntry = "100644 blob $ExpectedBlobOid`t$CatalogBlobPath"
	if ($treeEntry -ne $expectedEntry) { throw 'Pinned CONSTANTS_7 tree entry differs' }
	$blob = Read-GitBytes ("cat-file blob $ExpectedBlobOid") $gitDirectory
	if ($blob.Length -ne $CatalogBlobSize -or (Get-Sha256 $blob) -ne $ExpectedBlobSha256) {
		throw 'Pinned CONSTANTS_7 content identity differs'
	}
	$catalog = Read-Catalog $blob
	$contents = New-GeneratedInclude $catalog $Revision
	Publish-Atomically $Output $contents
} catch {
	[Console]::Error.WriteLine('Item catalog generation failed: ' + $_.Exception.Message)
	[Console]::Error.WriteLine($_.ScriptStackTrace)
	exit 1
}
