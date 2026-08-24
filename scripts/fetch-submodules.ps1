# Fetches git submodules a shallow (non-recursive) clone left empty. Runs from a PreBuildEvent -
# no-op once the markers exist. Mirrors cmake/FetchSubmodules.cmake; keep the two group lists in sync.
param([Parameter(Mandatory = $true)][string]$Root)

$ErrorActionPreference = 'Stop'

# Marker = a file that only exists once the group is checked out.
# Only SDL2_ttf actually has nested submodules (freetype, harfbuzz) worth recursing into.
$recursiveGroups = @(
    @{ Marker = 'ThirdParty/SDL2/CMakeLists.txt'; Paths = @('ThirdParty/SDL2') },
    @{ Marker = 'ThirdParty/SDL2_ttf/CMakeLists.txt'; Paths = @('ThirdParty/SDL2_ttf') },
    @{ Marker = 'ThirdParty/boost/uuid/CMakeLists.txt'; Paths = @('ThirdParty/boost') },
    @{ Marker = 'ThirdParty/ser20/CMakeLists.txt'; Paths = @('ThirdParty/ser20') },
    @{ Marker = 'ThirdParty/googletest/CMakeLists.txt'; Paths = @('ThirdParty/googletest') }
)

# SDL2IMAGE_VENDORED/SDL2MIXER_VENDORED are pinned OFF, so their external/* (dav1d, aom, libjxl,
# mpg123, fluidsynth with its test soundfonts - ~760 MB) is never configured. Don't clone it either.
$flatGroups = @(
    @{ Marker = 'ThirdParty/SDL2_image/CMakeLists.txt'; Paths = @('ThirdParty/SDL2_image') },
    @{ Marker = 'ThirdParty/SDL2_mixer/CMakeLists.txt'; Paths = @('ThirdParty/SDL2_mixer') }
)

# Collected first, then fetched in one pass per pool: the groups are independent, and a separate
# `git submodule update` per group would run them back to back, each with its own parallel pool.
function Get-MissingPaths($groups)
{
    return @($groups | Where-Object { -not (Test-Path (Join-Path $Root $_.Marker)) } | ForEach-Object { $_.Paths })
}

function Update-Submodules($paths, [switch]$Recurse)
{
    if (-not $paths) { return }

    #NOTE: --depth 1 here, not `shallow = true` in .gitmodules - see cmake/FetchSubmodules.cmake
    $flags = @('--init', '--jobs', '16', '--depth', '1') #NOTE: mostly tiny repos, cloning them one by one is network-latency bound
    if ($Recurse) { $flags += '--recursive' } # git has no --no-recursive, so the flag is added, never negated

    Write-Host "Submodule(s) $($paths -join ', ') are not initialized, fetching them now..."
    & git -C $Root submodule update @flags -- @paths
    if ($LASTEXITCODE -ne 0)
    {
        throw "Failed to fetch submodule(s) $($paths -join ', '). Run manually: git submodule update --init --recursive"
    }
}

Update-Submodules (Get-MissingPaths $recursiveGroups) -Recurse
Update-Submodules (Get-MissingPaths $flatGroups)
