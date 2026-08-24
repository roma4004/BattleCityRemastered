# Shrinks already-checked-out submodules to one commit of history. Maintenance, run by hand; fresh
# clones get --depth 1 from fetch-submodules.ps1 already.
#
# The refs have to go: gc keeps everything reachable from any ref, and a plain clone leaves
# refs/heads/<default> on the full-history tip. Submodules sit on a detached HEAD anyway.
param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot),
    [string[]]$Path = @(),
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

function Get-PackSize($repo)
{
    $line = & git -C $repo count-objects -v | Where-Object { $_ -like 'size-pack*' }
    if (-not $line) { return 0 }

    return [long]($line -split '\s+')[1] # KiB
}

function Format-Size([long]$kib)
{
    if ($kib -ge 1048576) { return '{0:N1} GiB' -f ($kib / 1048576) }
    if ($kib -ge 1024) { return '{0:N1} MiB' -f ($kib / 1024) }

    return "$kib KiB"
}

# ' <sha> <path> (<describe>)'; for a leading '-', '+' or 'U' the sha shown is not the pin.
function Get-Submodules
{
    $entries = @()
    foreach ($line in (& git -C $Root submodule status --recursive))
    {
        $state = $line.Substring(0, 1)
        $parts = $line.Substring(1).Trim() -split '\s+'
        $entries += [pscustomobject]@{ State = $state; Sha = $parts[0]; Path = $parts[1] }
    }

    if ($Path) { $entries = $entries | Where-Object { $Path -contains $_.Path } }

    return $entries
}

function Invoke-Shrink($entry)
{
    $repo = Join-Path $Root $entry.Path
    $before = Get-PackSize $repo

    if ($entry.State -eq '-')
    {
        Write-Host "  skip $($entry.Path): not initialized"
        return @{ Before = 0; After = 0 }
    }
    if ($entry.State -ne ' ')
    {
        Write-Warning "  skip $($entry.Path): checkout does not match the recorded pin ('$($entry.State)')"
        return @{ Before = $before; After = $before }
    }
    # tracked edits only - untracked leftovers and nested submodule state are not local changes
    if (& git -C $repo status --porcelain --untracked-files=no --ignore-submodules=all)
    {
        Write-Warning "  skip $($entry.Path): has local changes"
        return @{ Before = $before; After = $before }
    }

    $extraRefs = @(& git -C $repo for-each-ref --format='%(refname)' refs/remotes refs/tags refs/heads)
    $isShallow = (& git -C $repo rev-parse --is-shallow-repository) -eq 'true'
    if ($isShallow -and -not $extraRefs)
    {
        Write-Host "  ok   $($entry.Path): already minimal ($(Format-Size $before))"
        return @{ Before = $before; After = $before }
    }

    if ($DryRun)
    {
        Write-Host "  would shrink $($entry.Path): $(Format-Size $before), $($extraRefs.Count) ref(s) to drop"
        return @{ Before = $before; After = $before }
    }

    & git -C $repo fetch --quiet --depth 1 origin $entry.Sha
    if ($LASTEXITCODE -ne 0) { throw "Failed to fetch $($entry.Sha) for $($entry.Path)" }

    # the refs/heads deletion below must not cut the branch HEAD stands on
    & git -C $repo checkout --quiet --detach $entry.Sha
    if ($LASTEXITCODE -ne 0) { throw "Failed to check out $($entry.Sha) in $($entry.Path)" }

    foreach ($ref in $extraRefs) { & git -C $repo update-ref -d $ref }

    & git -C $repo reflog expire --expire=now --all
    & git -C $repo gc --quiet --prune=now

    $after = Get-PackSize $repo
    Write-Host "  $($entry.Path): $(Format-Size $before) -> $(Format-Size $after)"

    return @{ Before = $before; After = $after }
}

$entries = @(Get-Submodules)
if (-not $entries)
{
    Write-Host 'No submodules matched.'
    return
}

Write-Host "$(if ($DryRun) { 'Inspecting' } else { 'Shrinking' }) $($entries.Count) submodule(s) under $Root"

[long]$totalBefore = 0
[long]$totalAfter = 0
foreach ($entry in $entries)
{
    $result = Invoke-Shrink $entry
    $totalBefore += $result.Before
    $totalAfter += $result.After
}

Write-Host "Total: $(Format-Size $totalBefore) -> $(Format-Size $totalAfter)"
