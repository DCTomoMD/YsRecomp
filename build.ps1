# Builds the project (Release) in build\.
# Usage: .\build.ps1            -> build
#        .\build.ps1 -Test      -> build and run the quick tests ("quick" group, ~20 s)
#        .\build.ps1 -TestAll   -> build and run every test except the "history" group, several at a time
#                                  (before committing milestones that touch the console or the recompiler)
#        .\build.ps1 -TestHist  -> build and run the "history" group (old bugs in long sessions): when touching
#                                  smooth motion, crop, garbage frame or sound, and before large milestones
#        .\build.ps1 -Package   -> build, test the package ("package" group) and assemble the player package in
#                                  dist\ (YsRecomp\ and YsRecomp.zip)
#        -Verbose               -> show everything (full build and test output)
#        -VsPath FOLDER         -> another Visual Studio 2022 installation (the CI's Enterprise, found with
#                                  vswhere); without it, the Build Tools in their usual folder
#        -Edition usa|jp|all    -> edition of the game (default all: every edition whose disc and BIOS were found,
#                                  today only usa). The tests of the other editions are left out (common tests
#                                  always run); -Package builds the chosen editions' packages. jp: Phase 3B.
#                                  Which editions exist depends only on the disc and BIOS (YS_USA_CUE, YS_USA_BIOS;
#                                  cmake/ys_edition.cmake), not on this option.
# Without -Verbose, only build warnings and errors and the test summary (how many passed and which
# failed) are shown. To see the output of a failed test:
#   ctest --test-dir build -C Release -R <name> --output-on-failure
param([switch]$Test, [switch]$TestAll, [switch]$TestHist, [switch]$Package, [switch]$Verbose,
      [string]$VsPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools',
      [ValidateSet('usa', 'jp', 'all')][string]$Edition = 'all')
$ErrorActionPreference = 'Stop'

# Editions (tools/editions.py; the ctest label of each one's tests). jp comes in Phase 3B.
$available = @('usa')
$planned = @('jp')
if ($Edition -eq 'all') { $editions = $available }
elseif ($available -contains $Edition) { $editions = @($Edition) }
else { throw "Edition '$Edition' is not available yet (Phase 3B)" }
# Labels left out of the tests: the editions not chosen
$excluded = @(($available + $planned) | Where-Object { $editions -notcontains $_ })

$vs = $VsPath
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
if (-not (Test-Path $cmake)) { throw "CMake not found at $cmake" }

# Runs a program; without -Verbose, shows only the lines matching $filter.
function Run-Filtered([string]$exe, [string[]]$arguments, [string]$filter) {
    if ($Verbose) {
        & $exe @arguments | ForEach-Object { Write-Host $_ }
    } else {
        $out = & $exe @arguments 2>&1 | ForEach-Object { "$_" }
        $out | Select-String -Pattern $filter | ForEach-Object { Write-Host $_.Line }
    }
    return $LASTEXITCODE
}

Set-Location $PSScriptRoot
if (-not (Test-Path 'build\CMakeCache.txt')) {
    $rc = Run-Filtered $cmake @('-S', '.', '-B', 'build', '-G', 'Visual Studio 17 2022', '-A', 'x64') '(?i)error|warning'
    if ($rc -ne 0) { throw 'CMake configuration failed' }
}
$rc = Run-Filtered $cmake @('--build', 'build', '--config', 'Release', '--parallel') '(?i)warning C\d|error|: fatal'
if ($rc -ne 0) { throw 'Build failed' }
Write-Host 'Built.'

# Tests in parallel: half the cores (each test runs up to two programs at the same time).
$jobs = [Math]::Max(1, [int]([Environment]::ProcessorCount / 2))
if ($TestAll -or $Test -or $TestHist) {
    # Whatever fails runs once more (real-time tests can lose pacing when the computer is busy,
    # e.g. the user playing; a real defect fails both times)
    $targs = @('--test-dir', 'build', '-C', 'Release', '-j', $jobs, '--repeat', 'until-pass:2')
    if ($Verbose) { $targs += '--output-on-failure' }
    # (-L twice would mean "both labels" and drop the common tests: the other editions are excluded instead)
    $leftOut = @($excluded)
    if ($TestHist) { $targs += @('-L', 'history') }
    elseif ($TestAll) { $leftOut = @('history') + $leftOut }
    else { $targs += @('-L', 'quick') }
    if ($leftOut.Count -gt 0) { $targs += @('-LE', "^($($leftOut -join '|'))$") }
    $rc = Run-Filtered $ctest $targs '(?i)tests passed|\*\*\*Failed|\*\*\*Exception|\*\*\*Timeout|Not Run|^\s+\d+ - '
    if ($rc -ne 0) { throw 'Tests failed' }
}
if ($Package) {
    if (-not $TestAll) {
        $targs = @('--test-dir', 'build', '-C', 'Release', '-L', 'package', '--repeat', 'until-pass:2')
        if ($excluded.Count -gt 0) { $targs += @('-LE', "^($($excluded -join '|'))$") }
        if ($Verbose) { $targs += '--output-on-failure' }
        $rc = Run-Filtered $ctest $targs '(?i)tests passed|\*\*\*Failed|\*\*\*Exception|\*\*\*Timeout|Not Run|^\s+\d+ - '
        if ($rc -ne 0) { throw 'Package test failed' }
    }
    # One package per edition (the executable's name per edition comes in Phase 3B; usa: YsRecomp.exe)
    foreach ($ed in $editions) {
        & python tools\package\make_package.py --edition $ed build\Release\YsRecomp.exe dist
        if ($LASTEXITCODE -ne 0) { throw 'Package assembly failed' }
    }
}
