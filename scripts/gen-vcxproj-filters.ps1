# Regenerates <project>.vcxproj.filters so Solution Explorer mirrors the source tree.
# Display only, MSBuild ignores it. Runs from a PreBuildEvent - stays quiet and cheap.
param([Parameter(Mandatory = $true)][string]$Project)

$ns = 'http://schemas.microsoft.com/developer/msbuild/2003'
$xml = [xml](Get-Content -LiteralPath $Project -Raw)
$md5 = [System.Security.Cryptography.MD5]::Create()

# stable GUID per name - random ones would rewrite the whole file every run
function Get-FilterGuid([string]$name)
{
    $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes("vcxproj-filter/$name"))
    return "{$([guid]::new($bytes))}"
}

$items = @()
foreach ($tag in 'ClCompile', 'ClInclude')
{
    foreach ($node in $xml.GetElementsByTagName($tag, $ns))
    {
        $include = $node.GetAttribute('Include')
        if (-not $include)
        {
            continue
        }
        # project sits in a subfolder, so paths point outside it
        $dir = Split-Path -Parent $include
        while ($dir.StartsWith('..\'))
        {
            $dir = $dir.Substring(3)
        }
        $items += [pscustomobject]@{ Tag = $tag; Include = $include; Filter = $dir }
    }
}

# intermediate levels must be declared too, not just leaves
$filters = [System.Collections.Generic.SortedSet[string]]::new([StringComparer]::Ordinal)
foreach ($dir in ($items.Filter | Where-Object { $_ } | Select-Object -Unique))
{
    $parts = $dir.Split('\')
    for ($i = 1; $i -le $parts.Count; $i++)
    {
        [void]$filters.Add(($parts[0..($i - 1)] -join '\'))
    }
}

$out = [System.Text.StringBuilder]::new()
[void]$out.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
[void]$out.AppendLine("<Project ToolsVersion=`"4.0`" xmlns=`"$ns`">")
[void]$out.AppendLine('  <ItemGroup>')
foreach ($filter in $filters)
{
    [void]$out.AppendLine("    <Filter Include=`"$filter`">")
    [void]$out.AppendLine("      <UniqueIdentifier>$(Get-FilterGuid $filter)</UniqueIdentifier>")
    [void]$out.AppendLine('    </Filter>')
}
[void]$out.AppendLine('  </ItemGroup>')

foreach ($tag in 'ClCompile', 'ClInclude')
{
    $group = @($items | Where-Object { $_.Tag -eq $tag } | Sort-Object Include)
    if ($group.Count -eq 0)
    {
        continue
    }
    [void]$out.AppendLine('  <ItemGroup>')
    foreach ($item in $group)
    {
        [void]$out.AppendLine("    <$tag Include=`"$( $item.Include )`">")
        [void]$out.AppendLine("      <Filter>$( $item.Filter )</Filter>")
        [void]$out.AppendLine("    </$tag>")
    }
    [void]$out.AppendLine('  </ItemGroup>')
}
[void]$out.AppendLine('</Project>')

# write only on change - a touched timestamp would reload the project every build
$target = "$Project.filters"
$new = $out.ToString()
if ((-not (Test-Path -LiteralPath $target)) -or ((Get-Content -LiteralPath $target -Raw) -ne $new))
{
    [System.IO.File]::WriteAllText($target, $new, [System.Text.UTF8Encoding]::new($false))
    Write-Host "regenerated $( Split-Path -Leaf $target )"
}
