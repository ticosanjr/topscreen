# Build

## Instalação rápida

### Linux

```bash
# dependencias (uma vez)
sudo apt update && sudo apt install -y cmake build-essential libobs-dev
# opcional (dock): sudo apt install -y qt6-base-dev

./scripts/install-linux.sh            # plugin de usuario (sem sudo)
./scripts/install-linux.sh --system   # OBS do sistema (usa sudo)
```

### Windows

```powershell
# dependencias (uma vez): cmake + Visual Studio Build Tools
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools

# sem compilar aqui (use um topscreen.dll pronto):
pwsh -File scripts\install-windows.ps1 -Dll .\topscreen.dll

# compilar + instalar como plugin de usuario:
pwsh -File scripts\install-windows.ps1 -ObsDevDir C:\obs-dev

# ou instalar num OBS portable:
pwsh -File scripts\install-windows.ps1 -ObsDir D:\OBS
```

> **Windows:** compilar exige o **libobs dev** (headers + .lib). Se você só tem o
> OBS instalado, use `-Dll` com um binário já compilado, ou compile contra uma
> pasta de desenvolvimento do OBS com `-ObsDevDir`.

### Testes (nas duas plataformas)

```bash
./tests/run.sh                                   # Linux (g++ direto)
cmake -S . -B build-tests -DTOPSCREEN_BUILD_PLUGIN=OFF   # qualquer SO
cmake --build build-tests && ctest --test-dir build-tests --output-on-failure
```

```powershell
pwsh -File tests\run.ps1                         # Windows (ou PowerShell em qualquer SO)
```

> **Importante:** o log do OBS vai para o **stderr**. Use `2>&1`:
> `obs --verbose 2>&1 | grep -i topscreen`, ou veja
> `~/.config/obs-studio/logs/`.

## Dependências

- CMake >= 3.20
- Compilador com C++20 (MSVC 2019+, GCC 11+, Clang 13+)
- **libobs** (headers/libs) — venha do pacote de desenvolvimento do OBS, do build
  oficial de desenvolvimento, ou de um build local do OBS.
- **Qt6 Widgets** e **obs-frontend-api** (opcionais; necessários para o dock)
- X11 (`libx11-dev`) no Linux para o backend de input X11
- `linux/input.h` (linux-libc-dev) para o backend evdev (Tier 3)

> Este repositório **não** empacota as dependências. O `CMakeLists.txt` procura
> `libobs` via `find_package(libobs)`; se não achar, cai para `pkg-config libobs`.

---

## Linux

### 1. Obter o libobs

**Opção A — pacotes da distro** (quando existirem headers de plugin):

```bash
# Debian/Ubuntu (exemplo; nomes variam por versão)
sudo apt install cmake build-essential qt6-base-dev libx11-dev libobs-dev
```

**Opção B — build do OBS a partir do fonte** (recomendado para dev de plugin,
garante headers, `obs-frontend-api` e configs CMake):

```bash
git clone --recursive https://github.com/obsproject/obs-studio
cd obs-studio
cmake --preset ubuntu-x86_64
cmake --build build_x86_64 -j
cmake --install build_x86_64 --prefix "$HOME/obs-dev"
```

Aponte o TopScreen para essa instalação:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$HOME/obs-dev"
```

### 2. Compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 3. Instalar / testar

Instalação para o OBS do sistema:

```bash
sudo cmake --install build
# lib:  /usr/local/lib/obs-plugins/topscreen.so
# data: /usr/local/share/obs/obs-plugins/topscreen/
```

Modo **portable** (sem instalar): coloque ao lado do binário do OBS:

```bash
cp build/topscreen.so   <obs-portable>/obs-plugins/
cp -r data/             <obs-portable>/obs-plugins/topscreen/
```

Rode o OBS com logs e confirme o carregamento:

```bash
obs --verbose 2>&1 | grep -i topscreen
```

---

## Windows

1. Instale **Visual Studio 2022** (Desktop C++) e **CMake**.
2. Obtenha o **libobs + obs-frontend-api** (build de desenvolvimento do OBS ou
   compile o OBS com `cmake --preset windows-x64`).
3. Configure apontando para o OBS/Qt:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:\obs-dev"
cmake --build build --config Release
```

4. Copie o resultado para o OBS (portable ou instalado):

```
build\Release\topscreen.dll   ->  <obs>\obs-plugins\64bit\
data\                         ->  <obs>\obs-plugins\64bit\topscreen\
```

---

## Opções de CMake

| Opção | Padrão | Efeito |
|-------|--------|--------|
| `ENABLE_QT` | `ON` | Compila o dock Qt |
| `ENABLE_FRONTEND_API` | `ON` | Usa `obs-frontend-api` (necessário para o dock) |
| `TOPSCREEN_BUILD_TESTS` | `ON` | Compila os testes do núcleo (não dependem do OBS) |
| `CMAKE_BUILD_TYPE` | `Release` | Tipo de build em geradores single-config |

Defines automáticos: `TOPSCREEN_VERSION`, `TOPSCREEN_HAVE_QT`,
`TOPSCREEN_HAVE_FRONTEND`, `TOPSCREEN_HAVE_X11`, `TOPSCREEN_HAVE_EVDEV`.

---

## Testes

Os testes do **núcleo** não dependem de libobs/cmake e rodam em qualquer lugar:

```bash
./tests/run.sh          # compila com g++ e roda
```

Com CMake:

```bash
cmake -S . -B build -DTOPSCREEN_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Veja [`TESTES.md`](TESTES.md) para a cobertura e o que ainda precisa de um OBS real.

Para checar que o plugin **compila** contra os headers do OBS sem tê-lo instalado:

```bash
./tools/check-obs-headers.sh
```

---

## Troubleshooting

- **`find_package(libobs)` falha**: defina `-DCMAKE_PREFIX_PATH` para a pasta do
  OBS dev, ou garanta um `libobs.pc` no `PKG_CONFIG_PATH`.
- **Símbolos indefinidos ao carregar**: normalmente é mismatch de ABI entre o
  libobs usado no build e o OBS em execução — recompile contra a mesma versão.
- **Dock não aparece**: verifique se Qt6 e `obs-frontend-api` foram encontrados
  (o configure imprime `TopScreen: Qt=... frontend=...`).
- **Input não funciona no Linux**: veja `docs/INPUT-E-PLATAFORMAS.md`
  (X11 habilitado via `TOPSCREEN_HAVE_X11`; evdev exige permissão).
