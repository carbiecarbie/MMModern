param(
	[Parameter(Mandatory = $true)][string]$Source,
	[Parameter(Mandatory = $true)][string]$Revision,
	[Parameter(Mandatory = $true)][string]$Git,
	[Parameter(Mandatory = $true)][string]$ExpectedBlobOid,
	[Parameter(Mandatory = $true)][string]$ExpectedBlobSha256,
	[Parameter(Mandatory = $true)][string]$Output,
	[switch]$DialogText
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

# Named bounded fields in LangConstants::writeConstants at the pinned revision.
# Offsets include array count tags; no source text or worktree file is consumed.
function Test-DialogControls([byte[]]$Token, [string]$Name) {
	# FontSurface::writeString/getNextCharWidth at the pin. Inspect every token,
	# including templates reserved for Part B, without checking strings into source.
	for ($at = 0; $at -lt $Token.Length;) {
		$code = $Token[$at++] -band 127
		if ($code -ge 32) { continue }
		if ($code -in @(1,2,5,6,10,13)) { continue }
		if ($code -in @(3,8)) {
			if ($at -ge $Token.Length) { throw "Truncated dialog control: $Name" }
			++$at; continue
		}
		if ($code -notin @(4,7,9,11,12)) { throw "Unhandled dialog control $code in $Name" }
		$digits = if ($code -eq 12) { 2 } else { 3 }
		if ($at -lt $Token.Length -and $Token[$at] -eq 37) {
			$remaining = [Text.Encoding]::ASCII.GetString($Token,$at,$Token.Length-$at)
			$placeholder = [regex]::Match($remaining,'^%0?([1-3])[dui]')
			if (!$placeholder.Success -or [int]$placeholder.Groups[1].Value -ne $digits) { throw "Invalid dialog control placeholder: $Name" }
			$at += $placeholder.Length; continue
		}
		for ($digit = 0; $digit -lt $digits; ++$digit) {
			if ($at -ge $Token.Length) { throw "Truncated dialog control parameter: $Name" }
			$value = $Token[$at++]
			if ($code -eq 12 -and $digit -eq 0 -and $value -eq 100) { break }
			if ($value -ne 32 -and ($value -lt 48 -or $value -gt 57)) { throw "Invalid dialog control parameter: $Name" }
		}
	}
}
function New-DialogInclude([byte[]]$Bytes, [string]$SourceRevision) {
	$manifest = @'
ON_WHO 30462 30476 1
IN_NO_CONDITION 2282 2333 1
THE_PARTY_NEEDS_REST 1943 1969 1
RACE_NAMES 4555 4587 5
CLASS_NAMES 4655 4734 11
SEX_NAMES 4800 4816 2
SKILL_NAMES 4816 5035 18
CONDITION_NAMES_M 5035 5178 17
CONDITION_NAMES_F 5178 5321 17
GOOD 5393 5398 1
BLACKSMITH_TEXT 13906 14004 1
TEMPLE_TEXT 14825 14976 1
EXPERIENCE_FOR_LEVEL 14976 15013 1
TRAINING_LEARNED_ALL 15013 15046 1
ELIGIBLE_FOR_LEVEL 15046 15094 1
TRAINING_TEXT 15094 15169 1
GOLD_GEMS 15169 15254 1
NOT_ENOUGH_X_IN_THE_Y 15347 15380 1
STAT_NAMES 15405 15550 16
CONSUMABLE_NAMES 15550 15579 4
WHERE_NAMES 15589 15604 2
CHARACTER_DETAILS 18594 18934 1
DAYS 18934 18942 3
PARTY_GOLD 18942 18953 1
CHARACTER_TEMPLATE 18958 19224 1
EXCHANGING_IN_COMBAT 19224 19271 1
CURRENT_MAXIMUM_RATING_TEXT 19271 19327 1
CURRENT_MAXIMUM_TEXT 19327 19370 1
RATING_TEXT 19370 19586 24
BORN 19586 19596 2
AGE_TEXT 19596 19654 1
LEVEL_TEXT 19654 19718 1
RESISTENCES_TEXT 19718 19828 1
NONE 19828 19838 1
EXPERIENCE_TEXT 19838 19888 1
ELIGIBLE 19888 19902 1
IN_PARTY_IN_BANK 19902 19939 1
FOOD_ON_HAND 19939 19953 3
FOOD_TEXT 19953 19993 1
ITEMS_DIALOG_TEXT1 20329 20424 1
ITEMS_DIALOG_LINE1 20504 20529 1
ITEMS_DIALOG_LINE2 20529 20562 1
BTN_BUY 20562 20571 1
BTN_SELL 20571 20581 1
BTN_IDENTIFY 20581 20595 1
BTN_FIX 20595 20604 1
BTN_USE 20604 20613 1
BTN_EQUIP 20613 20624 1
BTN_REMOVE 20624 20633 1
BTN_DISCARD 20633 20643 1
BTN_QUEST 20643 20654 1
NOT_PROFICIENT 26254 26298 1
NO_ITEMS_AVAILABLE 26298 26325 1
CATEGORY_NAMES 26325 26369 4
X_FOR_THE_Y 26369 26428 1
X_FOR_Y 26428 26488 1
FMT_CHARGES 26555 26572 1
AVAILABLE_GOLD_COST 26572 26650 1
COST 26658 26663 1
GOLDS 27402 27412 2
ITEM_ACTIONS 26663 26714 7
WHICH_ITEM 26714 26737 1
WHATS_YOUR_HURRY 26737 26791 1
USE_ITEM_IN_COMBAT 26791 26858 1
NO_SPECIAL_ABILITIES 26858 26894 1
CANT_CAST_WHILE_ENGAGED 26894 26929 1
EQUIPPED_ALL_YOU_CAN 26929 26974 1
REMOVE_X_TO_EQUIP_Y 26974 27012 1
RING 27012 27017 1
MEDAL 27017 27023 1
CANNOT_REMOVE_CURSED_ITEM 27023 27057 1
PERMANENTLY_DISCARD 27094 27130 1
BACKPACK_IS_FULL 27130 27161 1
CATEGORY_BACKPACK_IS_FULL 27161 27337 4
BUY_X_FOR_Y_GOLD 27337 27369 1
SELL_X_FOR_Y_GOLD 27369 27402 1
ITEM_NOT_BROKEN 27524 27551 1
FIX_IDENTIFY 27551 27568 2
FIX_IDENTIFY_GOLD 27568 27597 1
'@
	$builder = New-Object Text.StringBuilder
	Add-Line $builder "/* ScummVM $SourceRevision; GPL-3.0-or-later; ScummVM developers (COPYRIGHT). Generated privately; see docs/dependencies.md. */"
	Add-Line $builder 'namespace mmodern::generated_dialog_text {'
	Add-Line $builder 'inline constexpr unsigned kSchema = 1, kLanguage = 7;'
	$names = @{}
	$script:TokenLimit = 512
	foreach ($line in ($manifest -split "`n")) {
		$fields = $line.Trim() -split ' '
		$name = $fields[0]; $position = [int]$fields[1]; $end = [int]$fields[2]; $count = [int]$fields[3]
		if ($name -notmatch '^[A-Z_][A-Z_0-9]+$' -or $names.ContainsKey($name) -or $count -lt 1 -or $count -gt 24 -or $end -gt $Bytes.Length) {
			throw 'Invalid dialog manifest name/count/bounds'
		}
		$names[$name] = $true
		$script:CatalogEnd = $end
		if ($count -gt 1) {
			if ($Bytes[$position] -ne 0 -or $Bytes[$position+1] -ne 0 -or $Bytes[$position+2] -ne 0 -or $Bytes[$position+3] -ne $count) {
				throw "Dialog array count mismatch: $name"
			}
			$position += 4
			Add-Line $builder "inline constexpr std::array<std::string_view, $count> $name{{"
		}
		for ($index = 0; $index -lt $count; ++$index) {
			$token = Read-CatalogToken $Bytes ([ref]$position) "$name[$index]"
			# Plural arrays deliberately contain unused empty English forms.
			if (!$token.Length -and $name -notin @('DAYS','BORN','FOOD_ON_HAND','GOLDS') -and !($name -eq 'CLASS_NAMES' -and $index -eq 10)) { throw "Missing dialog template: $name" }
			Test-DialogControls $token $name
			# The pinned FMT_CHARGES contains a second literal alignment letter.
			# The maintainer's DOSBox reference shows no extra title glyph; see the
			# recorded M47 deviation. Preserve verified input, correct only output.
			if ($name -eq 'FMT_CHARGES' -and $token.Length -gt 3 -and
				$token[0] -eq 3 -and $token[1] -eq 114 -and $token[2] -eq 114 -and $token[3] -eq 9) {
				$token = [byte[]]($token[0..1] + $token[3..($token.Length-1)])
			}
			$literal = ConvertTo-OctalLiteral $token
			if ($count -eq 1) { Add-Line $builder "inline constexpr std::string_view $name = $literal;" }
			else { Add-Line $builder ("`t" + $literal + ',') }
		}
		if ($position -ne $end) { throw "Dialog block end mismatch: $name" }
		if ($count -gt 1) { Add-Line $builder '}};' }
	}
	if ($names.Count -ne 79) { throw 'Dialog template name count mismatch' }
	# The original window border and four-shade font palettes are numeric drawing
	# inputs from the same verified stream, not strings or commercial assets.
	foreach ($table in @(@('WindowSymbols',2891,20,64), @('TextColors',4175,40,4))) {
		$position = [int]$table[1]; $rows = [int]$table[2]; $columns = [int]$table[3]
		if ($Bytes[$position] -ne 0 -or $Bytes[$position+1] -ne 0 -or $Bytes[$position+2] -ne $columns -or $Bytes[$position+3] -ne $rows) {
			throw 'Dialog drawing array tag mismatch'
		}
		Add-Line $builder "inline constexpr std::array<std::array<unsigned char, $columns>, $rows> k$($table[0]){{"
		$position += 4
		for ($row = 0; $row -lt $rows; ++$row) {
			$values = @($Bytes[$position..($position+$columns-1)] | ForEach-Object { [string]$_ })
			Add-Line $builder ('{{' + ($values -join ',') + '}},')
			$position += $columns
		}
		Add-Line $builder '}};'
	}
	Add-Line $builder '} // namespace mmodern::generated_dialog_text'
	if ($builder.Length -gt 65536) { throw 'Dialog include exceeds 65,536 bytes' }
	return $builder.ToString()
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
	if ($DialogText) { $contents = New-DialogInclude $blob $Revision }
	else {
		$catalog = Read-Catalog $blob
		$contents = New-GeneratedInclude $catalog $Revision
	}
	Publish-Atomically $Output $contents
} catch {
	[Console]::Error.WriteLine('Item catalog generation failed: ' + $_.Exception.Message)
	[Console]::Error.WriteLine($_.ScriptStackTrace)
	exit 1
}
