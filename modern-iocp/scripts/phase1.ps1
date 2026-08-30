# S1 원커맨드 래퍼. 사용법: .\scripts\phase1.ps1 <doctor|build|test|run|crash>
#
# 이 스크립트는 cmake 호출을 감싸기만 한다. 빌드 설정은 CMakePresets.json 이 정본이고
# 여기서 컴파일 플래그나 경로를 다시 정의하지 않는다.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('doctor', 'build', 'test', 'run', 'crash', 'verify-dump')]
    [string]$Command,

    # CI 러너에는 로컬과 같은 toolset 이 없다. x64-release-ci 는 그것을 고정하지 않는다.
    [Parameter()]
    [ValidateSet('x64-release', 'x64-release-ci')]
    [string]$Preset = 'x64-release',

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rest
)

$ErrorActionPreference = 'Stop'

$PresetName = $Preset
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build\$PresetName"
$ServerExe = Join-Path $BuildDir 'Release\GameServer.exe'

# CMakePresets.json 이 요구하는 값. doctor 가 이것과 실제 환경을 대조한다.
$RequiredToolset = '14.51'
$RequiredSdk = '10.0.26100.0'

function Fail($message) {
    Write-Host "[FAIL] $message" -ForegroundColor Red
    exit 1
}

function Ok($message) {
    Write-Host "[ ok ] $message" -ForegroundColor Green
}

function Get-VsInstallPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { return $null }
    $path = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ([string]::IsNullOrWhiteSpace($path)) { return $null }
    return $path.Trim()
}

function Invoke-Doctor {
    # fresh clone 에서 빌드가 깨졌을 때 "환경 탓인가 저장소 탓인가"를 가르는 것이 목적이다.
    # 부족한 것을 하나씩 이름으로 말해야 한다. 뭉뚱그린 실패는 도움이 안 된다.
    $problems = @()

    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($null -eq $cmake) {
        $problems += 'cmake 가 PATH 에 없다. https://cmake.org 에서 설치하거나 VS Installer 의 "C++ CMake tools for Windows" 를 켜라.'
    }
    else {
        Ok "cmake: $((& cmake --version | Select-Object -First 1))"
    }

    $vsPath = Get-VsInstallPath
    if ($null -eq $vsPath) {
        $problems += 'C++ 도구가 있는 Visual Studio 를 찾지 못했다. VS Installer 에서 "Desktop development with C++" 를 설치하라.'
    }
    else {
        Ok "Visual Studio: $vsPath"

        $toolsetFile = Join-Path $vsPath 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
        if (Test-Path $toolsetFile) {
            $toolset = (Get-Content $toolsetFile -First 1).Trim()
            if ($toolset.StartsWith($RequiredToolset)) {
                Ok "MSVC toolset: $toolset"
            }
            else {
                $problems += "MSVC toolset 이 $toolset 다. CMakePresets.json 은 $RequiredToolset 계열을 요구한다. VS Installer 에서 해당 toolset 을 설치하거나 preset 을 갱신하라."
            }
        }
        else {
            $problems += "MSVC toolset 버전 파일을 찾지 못했다: $toolsetFile"
        }
    }

    $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
    if ((Test-Path $sdkRoot) -and (Test-Path (Join-Path $sdkRoot $RequiredSdk))) {
        Ok "Windows SDK: $RequiredSdk"
    }
    else {
        $found = if (Test-Path $sdkRoot) { (Get-ChildItem $sdkRoot -Directory | ForEach-Object Name) -join ', ' } else { '(없음)' }
        $problems += "Windows SDK $RequiredSdk 가 없다. 설치된 것: $found. VS Installer 에서 해당 SDK 를 추가하거나 preset 을 갱신하라."
    }

    if ([string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
        $problems += 'VCPKG_ROOT 환경변수가 없다. vcpkg 를 clone·bootstrap 한 뒤 그 경로를 VCPKG_ROOT 로 설정하고 셸(또는 Visual Studio)을 재시작하라.'
    }
    elseif (-not (Test-Path (Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake'))) {
        $problems += "VCPKG_ROOT=$env:VCPKG_ROOT 에 vcpkg 툴체인 파일이 없다. bootstrap-vcpkg.bat 를 실행했는지 확인하라."
    }
    else {
        Ok "VCPKG_ROOT: $env:VCPKG_ROOT"

        # vcpkg.json 의 builtin-baseline 커밋이 로컬 vcpkg 저장소에 실제로 있어야
        # 선언한 버전으로 복원된다. 없으면 다른 버전이 설치돼 재현성이 깨진다.
        $manifest = Get-Content (Join-Path $RepoRoot 'vcpkg.json') -Raw | ConvertFrom-Json
        $baseline = $manifest.'builtin-baseline'
        & git -C $env:VCPKG_ROOT cat-file -e "$baseline^{commit}" 2>$null
        if ($LASTEXITCODE -eq 0) {
            Ok "vcpkg baseline: $baseline"
        }
        else {
            $problems += "vcpkg baseline 커밋 $baseline 이 로컬 vcpkg 저장소에 없다. VCPKG_ROOT 에서 git fetch 를 하라."
        }
    }

    if ($problems.Count -gt 0) {
        Write-Host ''
        foreach ($p in $problems) { Write-Host "[FAIL] $p" -ForegroundColor Red }
        exit 1
    }

    Write-Host ''
    Ok '환경 검사 통과. .\scripts\phase1.ps1 build 를 실행할 수 있다.'
}

function Invoke-Build {
    Push-Location $RepoRoot
    try {
        & cmake --preset $PresetName
        if ($LASTEXITCODE -ne 0) { Fail "configure 실패 (exit $LASTEXITCODE). 원인이 환경이면 .\scripts\phase1.ps1 doctor 를 먼저 돌려라." }

        & cmake --build --preset $PresetName
        if ($LASTEXITCODE -ne 0) { Fail "build 실패 (exit $LASTEXITCODE)" }
    }
    finally { Pop-Location }
    Ok "빌드 완료: $BuildDir\Release"
}

function Invoke-Test {
    if (-not (Test-Path $BuildDir)) { Fail "빌드 디렉터리가 없다. 먼저 .\scripts\phase1.ps1 build 를 실행하라." }
    Push-Location $RepoRoot
    try {
        & ctest --preset $PresetName
        if ($LASTEXITCODE -ne 0) { Fail "테스트 실패 (exit $LASTEXITCODE)" }
    }
    finally { Pop-Location }
    Ok '테스트 통과'
}

function Assert-ServerBuilt {
    if (-not (Test-Path $ServerExe)) {
        Fail "서버 실행 파일이 없다: $ServerExe`n먼저 .\scripts\phase1.ps1 build 를 실행하라."
    }
}

function Invoke-Run {
    Assert-ServerBuilt
    & $ServerExe @Rest
    exit $LASTEXITCODE
}

function Invoke-Crash {
    # 의도적 크래시. 크래시 계약상 실행 파일 안의 고정된 코드 경로가 예외를 낸다.
    # 여기서 프로세스를 밖에서 죽이지 않는다 - 그러면 크래시 핸들러를 검증하지 못한다.
    Assert-ServerBuilt
    & $ServerExe --crash-test
    # 크래시가 나는 것이 정상 동작이므로 그 종료 코드를 실패로 전파하지 않는다.
    # 판정은 verify-dump 가 한다.
    Write-Host "GameServer.exe --crash-test 종료 코드: $LASTEXITCODE (0xC0000005 = 액세스 위반, 의도된 값)"
    exit 0
}

function Invoke-VerifyDump {
    # "덤프 plumbing 이 살아 있다" 를 사람 눈이 아니라 exit code 로 판정한다.
    # 이 서브커맨드가 S1 완료 조건의 기계적 근거다. CI 도 이것을 실행한다.
    Assert-ServerBuilt

    $output = & $ServerExe --crash-test 2>&1 | Out-String
    Write-Host $output.TrimEnd()

    # CrashHandler.cpp 가 찍는 줄. 형식이 바뀌면 여기도 바꿔야 한다.
    $match = [regex]::Match($output, '\[CRASH\] dump written:\s*(.+?)\s*$', 'Multiline')
    if (-not $match.Success) {
        Fail "크래시 핸들러가 덤프 경로를 출력하지 않았다. CrashHandler 가 설치되지 않았거나 덤프 작성에 실패했다."
    }

    $dumpPath = $match.Groups[1].Value.Trim()
    if (-not (Test-Path -LiteralPath $dumpPath)) {
        Fail "덤프 경로가 출력됐지만 파일이 없다: $dumpPath"
    }

    $dumpSize = (Get-Item -LiteralPath $dumpPath).Length
    if ($dumpSize -le 0) {
        Fail "덤프 파일이 비어 있다: $dumpPath"
    }
    Ok "덤프 생성: $dumpPath ($dumpSize bytes)"

    # 덤프만 있으면 주소만 보인다. 심볼을 붙이려면 짝이 되는 PDB 가 있어야 한다.
    $pdbPath = [System.IO.Path]::ChangeExtension($ServerExe, '.pdb')
    if (-not (Test-Path -LiteralPath $pdbPath)) {
        Fail "대응 PDB 가 없다: $pdbPath`nRelease 빌드에 /Zi 와 링커 /DEBUG 가 걸려 있는지 확인하라."
    }
    Ok "대응 PDB: $pdbPath ($((Get-Item -LiteralPath $pdbPath).Length) bytes)"

    Write-Host ''
    Ok '덤프 plumbing 검증 통과'
    # 자식 프로세스가 크래시로 죽었으므로 $LASTEXITCODE 에 0xC0000005 가 남아 있다.
    # 명시적으로 0 을 반환하지 않으면 CI 가 이 성공을 실패로 읽는다.
    exit 0
}

switch ($Command) {
    'doctor' { Invoke-Doctor }
    'build' { Invoke-Build }
    'test' { Invoke-Test }
    'run' { Invoke-Run }
    'crash' { Invoke-Crash }
    'verify-dump' { Invoke-VerifyDump }
}
