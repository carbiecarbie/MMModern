param($Path,$CMake,$Generator,$Source,$Revision,$Git,$Oid,$Sha)
$ErrorActionPreference='Stop'
$lock=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
try {
 & $CMake "-DSource=$Source" "-DRevision=$Revision" "-DGit=$Git" "-DExpectedBlobOid=$Oid" "-DExpectedBlobSha256=$Sha" "-DOutput=$Path" -P $Generator
 if($LASTEXITCODE -eq 0){exit 1}
} finally {$lock.Dispose()}
exit 0
