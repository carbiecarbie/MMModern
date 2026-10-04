param(
	[Parameter(Mandatory = $true)][string]$Generator,
	[Parameter(Mandatory = $true)][string]$TestPowerShell,
	[Parameter(Mandatory = $true)][string]$Git,
	[Parameter(Mandatory = $true)][string]$TestRoot,
	[Parameter(Mandatory = $true)][string]$ProductionOutput,
	[Parameter(Mandatory = $false)][string]$ProductionOutputSha256
)

$ErrorActionPreference = 'Stop'
$CatalogSize = 35065
$BlobPath = 'devtools/create_mm/files/xeen/CONSTANTS_7'

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
		'-Output', $Output, '-DialogText')
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

function New-DialogBlob([string]$Mode='valid') {
 $blob=New-Object byte[] 35065
 $generatorText=[IO.File]::ReadAllText($Generator)
 $manifest=[regex]::Match($generatorText,"(?s)\`$manifest = @'\r?\n(.*?)\r?\n'@").Groups[1].Value
 Check ([bool]$manifest) 'Dialog manifest unavailable to fixture builder'
 foreach($line in ($manifest -split "`n")) {
  $parts=$line.Trim() -split ' ';$start=[int]$parts[1];$end=[int]$parts[2];$count=[int]$parts[3]
  if($count -gt 1){$blob[$start+3]=$count;$start+=4}
  for($index=0;$index -lt $count;++$index){
   $length=[Math]::Floor(($end-$start)/($count-$index))-1
   for($n=0;$n -lt $length;++$n){$blob[$start+$n]=120}
   $start+=$length+1
  }
 }
 $blob[2893]=64;$blob[2894]=20;$blob[4177]=4;$blob[4178]=40
 # Synthetic charges field exercises the output-only redundant-letter fix.
 $blob[26555]=3;$blob[26556]=114;$blob[26557]=114;$blob[26558]=9
 $blob[26559]=48;$blob[26560]=48;$blob[26561]=48;$blob[26569]=3;$blob[26570]=108
 if($Mode -eq 'missing'){$blob[18594]=0}
 if($Mode -eq 'oversized'){for($n=18594;$n -lt 18934;++$n){$blob[$n]=120}}
 if($Mode -eq 'bad-count'){$blob[4819]=19}
 if($Mode -eq 'bad-control'){$blob[18594]=14}
 if($Mode -eq 'bad-parameter'){$blob[18594]=9;$blob[18595]=120}
 if($Mode -eq 'truncated'){return ,$blob[0..35063]}
 return ,$blob
}
 $fixtureRoot=[IO.Path]::GetFullPath($TestRoot)
try {
 $TestRoot=Join-Path $TestRoot ('run-'+[Guid]::NewGuid().ToString('N'))
 [IO.Directory]::CreateDirectory($TestRoot)|Out-Null
 $valid=New-TestRepository 'valid' (New-DialogBlob)
 $output=Join-Path $TestRoot 'dialog.inc'
 Invoke-Generator $valid $output $true
 $first=Get-Sha256 ([IO.File]::ReadAllBytes($output));$timestamp=[IO.File]::GetLastWriteTimeUtc($output)
 Invoke-Generator $valid $output $true
 Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq $first) 'Dialog output nondeterministic'
 Check ([IO.File]::GetLastWriteTimeUtc($output) -eq $timestamp) 'Unchanged output replaced'
 $text=[IO.File]::ReadAllText($output)
 Check ($text.Contains('CHARACTER_DETAILS') -and $text.Contains('ON_WHO') -and $text.Contains('COST') -and $text.Contains('GOLDS') -and $text.Contains('kWindowSymbols')) 'Required generated fields missing'
 Check ($text.Contains('\003\162\011\060\060\060') -and !$text.Contains('\003\162\162\011')) 'Charges field kept the redundant literal glyph'
 [IO.File]::WriteAllBytes($valid.Input,(New-Object byte[] 35065))
 Invoke-Generator $valid $output $true
 Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq $first) 'Worktree influenced dialog output'
 Invoke-Generator $valid $output $false ('0'*40)
 Invoke-Generator $valid $output $false $valid.Revision ('0'*40)
 Invoke-Generator $valid $output $false $valid.Revision $valid.Oid ('0'*64)
 foreach($mode in @('missing','oversized','bad-count','bad-control','bad-parameter','truncated')){
  $invalid=New-TestRepository $mode (New-DialogBlob $mode)
  Invoke-Generator $invalid $output $false
  Check ((Get-Sha256 ([IO.File]::ReadAllBytes($output))) -eq $first) 'Failed generation changed output'
 }
 Check ([IO.File]::ReadAllText($ProductionOutput).Contains('kLanguage = 7')) 'Production dialog include unavailable'
 Write-Output 'Dialog generation determinism, blob-only input and rejection checks passed'
} finally {
 if($TestRoot -and (Test-Path -LiteralPath $TestRoot)){
  $resolvedFixture=(Resolve-Path -LiteralPath $TestRoot).Path
  $fixturePrefix=$fixtureRoot.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
  Check ($resolvedFixture.StartsWith($fixturePrefix,[StringComparison]::OrdinalIgnoreCase)) 'Fixture cleanup escaped test root'
  Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
 }
}
