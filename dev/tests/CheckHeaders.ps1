[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$developmentRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$includeRoot = Join-Path $developmentRoot 'include'
$outputRoot = [IO.Path]::GetFullPath((Join-Path $developmentRoot 'obj\HeaderChecks'))
if (-not $outputRoot.StartsWith($developmentRoot + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Header check output must remain inside dev.'
}
[void][IO.Directory]::CreateDirectory($outputRoot)

# Capture child-process output without opening a terminal window or changing the
# caller's environment. In particular, never print the environment returned by set.
function Invoke-CapturedProcess {
    param(
        [string]$Executable,
        [string]$Arguments,
        [hashtable]$ChildEnvironment
    )
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $Executable
    $startInfo.Arguments = $Arguments
    $startInfo.WorkingDirectory = $outputRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    if ($ChildEnvironment) {
        foreach ($entry in $ChildEnvironment.GetEnumerator()) {
            $startInfo.EnvironmentVariables[$entry.Key] = $entry.Value
        }
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = $stdout.Result
            ErrorOutput = $stderr.Result
        }
    } finally {
        $process.Dispose()
    }
}

$installerRoot = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
$vswhere = Join-Path $installerRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    $vswhere = Join-Path $env:ProgramFiles 'Microsoft Visual Studio\Installer\vswhere.exe'
}
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer vswhere.exe was not found.' }
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'A Visual Studio installation with the C++ x64 tools is required.' }
$installation = @($installation)[0]
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars)) { throw 'The selected Visual Studio installation has no vcvars64.bat.' }
$environmentResult = Invoke-CapturedProcess $env:ComSpec ('/d /s /c "call "' + $vcvars + '" >nul && set"')
if ($environmentResult.ExitCode -ne 0) { throw 'Visual Studio x64 environment initialization failed.' }
$compilerEnvironment = @{}
foreach ($line in $environmentResult.Output -split '\r?\n') {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $compilerEnvironment[$line.Substring(0, $separator)] = $line.Substring($separator + 1)
    }
}
if (-not $compilerEnvironment.ContainsKey('VCToolsInstallDir')) { throw 'Visual Studio did not provide VCToolsInstallDir.' }
$compiler = Join-Path $compilerEnvironment['VCToolsInstallDir'] 'bin\Hostx64\x64\cl.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw 'The selected x64 MSVC compiler was not found.' }

# Parsing a header alone does not instantiate an implicit destructor. These
# independent translation units also exercise public ownership/return contracts.
$construction = @{
    'Area.h' = 'Area area;'
    'Clue.h' = 'Clue clue;'
    'CombatManager.h' = 'CombatManager combat;'
    'CombatMath.h' = 'auto roll = CombatMath::SubstatRoll(ItemStat::ATK, Rarity::Common, 1u);'
    'CombatResolution.h' = 'DamageResolution damage;'
    'ConsoleUI.h' = 'ConsoleUI ui;'
    'ContentCatalog.h' = 'auto items = BuildEquippableCatalog();'
    'EncounterManager.h' = 'EncounterManager encounters; InvestigationResult investigation; VictoryResult victory;'
    'Enemy.h' = 'EnemyDefinition definition; Enemy enemy(definition, 1);'
    'EnemyCatalog.h' = 'auto enemy = CreateRandomCatalogueEnemy(1u, 1);'
    'GameManager.h' = 'GameManager game;'
    'GameplayTypes.h' = 'Stats stats; ItemDefinition item; EnemyDefinition enemy; RoomDefinition room;'
    'InteractionManager.h' = 'InteractionManager interactions;'
    'Inventory.h' = 'Inventory inventory; ItemEntry entry;'
    'Item.h' = 'Item item;'
    'MapManager.h' = 'MapManager map;'
    'Player.h' = 'Player player;'
    'ProgressionManager.h' = 'ProgressionManager progression;'
    'ShimmerSkill.h' = 'auto skill = ShimmerSkill::Create("Pulsus Luminis");'
}
$summary = New-Object 'System.Collections.Generic.List[string]'
$summary.Add('MSVC C++17 isolated header checks: ' + [DateTime]::UtcNow.ToString('u'))
$summary.Add('Compiler: ' + $compiler)
$summary.Add('Options: /nologo /EHsc /std:c++17 /permissive- /Zs /TP')
$failed = 0
$total = 0
foreach ($header in Get-ChildItem -LiteralPath $includeRoot -Filter '*.h' | Sort-Object Name) {
    $checks = [ordered]@{ include = '' }
    if ($construction.ContainsKey($header.Name)) { $checks['construction'] = $construction[$header.Name] }
    foreach ($check in $checks.GetEnumerator()) {
        ++$total
        $stem = $header.BaseName + '.' + $check.Key
        $fixture = Join-Path $outputRoot ($stem + '.cpp')
        $log = Join-Path $outputRoot ($stem + '.log')
        $source = '#include "' + $header.Name + '"' + "`r`nint main() { " + $check.Value + " return 0; }`r`n"
        [IO.File]::WriteAllText($fixture, $source, [Text.Encoding]::ASCII)
        # Windows file names cannot contain quotes. Both paths are quoted native
        # arguments, with no shell interpolation during compiler invocation.
        $arguments = '/nologo /EHsc /std:c++17 /permissive- /Zs /TP /I "' + $includeRoot + '" "' + $fixture + '"'
        $result = Invoke-CapturedProcess $compiler $arguments $compilerEnvironment
        [IO.File]::WriteAllText($log, $result.Output + $result.ErrorOutput, [Text.Encoding]::UTF8)
        if ($result.ExitCode -eq 0) {
            $message = 'PASS ' + $header.Name + ' [' + $check.Key + ']'
        } else {
            ++$failed
            $message = 'FAIL ' + $header.Name + ' [' + $check.Key + '] -> ' + $log
        }
        $summary.Add($message)
        Write-Output $message
    }
}
$conclusion = 'Header checks: ' + ($total - $failed) + '/' + $total + ' passed; ' + $failed + ' failed.'
$summary.Add($conclusion)
[IO.File]::WriteAllLines((Join-Path $outputRoot 'summary.txt'), $summary, [Text.Encoding]::UTF8)
Write-Output $conclusion
Write-Output ('Logs and isolated translation units: ' + $outputRoot)
if ($failed -gt 0) { exit 1 }
exit 0
