param([string]$Compiler = 'tcc')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out = Join-Path $repo 'build/boot-guard-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null

foreach ($name in @('policy', 'settings')) {
    $source = Join-Path $PSScriptRoot "test_boot_guard_$name.c"
    $exe = Join-Path $out "test_boot_guard_$name.exe"
    $compilerArgs = @('-Wall', '-Werror', '-o', $exe, $source)
    if ($name -eq 'settings') {
        $compilerArgs += @('-I', (Join-Path $PSScriptRoot 'boot_guard_stubs'),
            (Join-Path $repo 'main/boot_guard_settings.c'))
    }
    & $Compiler @compilerArgs
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $name" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $name" }
}
