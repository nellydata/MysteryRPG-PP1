[CmdletBinding()]
param([ValidatePattern('^phase-[0-9]+$')][string]$CaptureName = 'phase-2')
$ErrorActionPreference = 'Stop'
$developmentRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$executable = Join-Path $developmentRoot 'bin\x64\Debug\COS-PP1.exe'
$transcriptPath = Join-Path $developmentRoot ('validation\' + $CaptureName + '-console.txt')
if (-not (Test-Path -LiteralPath $executable)) { throw 'Build Debug x64 before the console smoke test.' }

# Only the real executable's stdin/stdout are used. These are numbered
# keyboard choices, with no test-only game flags or injected player state.
$choices = @(
    'ConsoleEconomySmoke',
    '6', '4', '2', '4', '1', '4', '2', '6', '1', '0',
    '1', '1', '1', '1', '2', '1', '1', '1',
    '7', '3', '1', '1', '1', '3', '0',
    '5', '1', '1', '0', '0', '2', '0', '0'
)
$process = New-Object System.Diagnostics.Process
$start = New-Object System.Diagnostics.ProcessStartInfo
$start.FileName = $executable
$start.Arguments = '--fast'
$start.WorkingDirectory = $developmentRoot
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.RedirectStandardInput = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$process.StartInfo = $start
try {
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.Refresh()
    $windowHandle = $process.MainWindowHandle
    if ($windowHandle -ne [IntPtr]::Zero) { throw 'Unexpected GUI window handle.' }
    foreach ($choice in $choices) { $process.StandardInput.WriteLine($choice) }
    $process.StandardInput.Close()
    if (-not $process.WaitForExit(30000)) { throw 'Console smoke exceeded 30 seconds.' }
    $output = $stdout.Result
    $errorOutput = $stderr.Result
    $normalized = (($output + $errorOutput) -split '\r?\n' | ForEach-Object { $_.TrimEnd() }) -join [Environment]::NewLine
    [IO.File]::WriteAllText($transcriptPath, $normalized, (New-Object Text.UTF8Encoding($false)))
    if ($process.ExitCode -ne 0 -or $errorOutput) { throw "Console exit $($process.ExitCode): $errorOutput" }
    foreach ($expected in @(
        'cellar_rations [Common] x1 [LOCKED]',
        'Cannot use this item here:',
        'Equipment updated: linen_veil',
        'ConsoleEconomySmoke uses cellar_rations on ConsoleEconomySmoke.',
        'The Fountain is purified.',
        'Entrance Hall is now unlocked.',
        'Found: faded_sanctum_gown.',
        'Main: DEF +11%',
        'DEF 10 -> 11',
        'Equipment updated: faded_sanctum_gown',
        'Equipped, locked, or story items cannot be discarded.',
        'The Fountain restores you to full health.',
        'Shimmer loadout updated.',
        '=== ROOM: Entrance Hall ===',
        'Session complete.'
    )) {
        if (-not $output.Contains($expected)) { throw "Console smoke missed: $expected" }
    }
    if ([regex]::Matches($output, 'uses cellar_rations on').Count -ne 1) { throw 'Consumable applied more than once.' }
    if ([regex]::Matches($output, 'The Fountain is purified\.').Count -ne 1) { throw 'Duplicate first-clear reward.' }
    $binary = [IO.File]::ReadAllBytes($executable)
    $peOffset = [BitConverter]::ToInt32($binary, 0x3c)
    $subsystem = [BitConverter]::ToUInt16($binary, $peOffset + 24 + 68)
    if ($subsystem -ne 3) { throw "Expected console PE subsystem 3, found $subsystem." }
    Write-Output "Console smoke passed: exit 0; PE subsystem $subsystem; GUI handle $windowHandle."
    Write-Output 'Keyboard paths: fresh initialization, exact item lock/use/unlock, equipment previews and equip, consumable combat use, ordinary attacks, single Fountain loot/XP/unlock, recovery, protected discard, Shimmer toggle, next-room entry and exit.'
    Write-Output "Transcript: $transcriptPath"
} finally {
    # This process belongs to this test; never stop unrelated user sessions.
    if ($process.Id -and -not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}
