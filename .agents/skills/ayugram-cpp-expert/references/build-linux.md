# Build do AyuGramDesktop para Linux — Referência Técnica

Fonte primária (sempre confira a versão mais atual antes de repassar ao usuário):
`https://github.com/AyuGram/AyuGramDesktop/blob/dev/docs/building-linux.md`

## 1. Visão geral do pipeline oficial

O build "oficial" (o que a própria equipe usa para gerar os binários publicados) é
feito **dentro de um container Docker CentOS** (`centos_env`), para garantir uma glibc
antiga o suficiente e máxima compatibilidade binária entre distros Linux. Isso é
importante explicar ao usuário: compilar "nativamente" no Ubuntu/Arch/Fedora do
usuário funciona para uso pessoal, mas não gera um binário tão portátil quanto o
oficial.

## 2. Passo a passo — fluxo Docker (recomendado para builds "de verdade")

### 2.1. Pré-requisitos
- `git`
- [`poetry`](https://python-poetry.org) (usado pelos scripts de preparo em Python)
- `docker` + `docker-buildx`
- Espaço em disco generoso (o build completo, com todas as libs, pode passar de
  20–30 GB) e **RAM/swap suficiente** — usuários relatam necessidade de 16 GB RAM +
  16 GB swap em máquinas com poucos núcleos; em máquinas com muitos núcleos o Ninja
  paraleliza agressivamente e pode esgotar a memória (ver seção de troubleshooting).

### 2.2. Clonar e preparar

```bash
# Escolha uma pasta de build, ex: ~/TBuild
cd ~/TBuild
git clone --recursive https://github.com/AyuGram/AyuGramDesktop.git tdesktop
./tdesktop/Telegram/build/prepare/linux.sh
```

`--recursive` é obrigatório: o projeto depende de vários submódulos Git (Qt, libs do
desktop-app framework, etc.). Se o clone dos submódulos travar/for muito lento:

```bash
export GIT_CONFIG_PARAMETERS="'submodule.fetchJobs=4'"
```

### 2.3. Compilar via Docker

A partir de `~/TBuild/tdesktop`:

```bash
docker run --rm -it \
    -u $(id -u) \
    -v "$PWD:/usr/src/tdesktop" \
    ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID=2040 \
    -D TDESKTOP_API_HASH=b18441a1ff607e10a989891a5462e627
```

Para build de **debug**:

```bash
docker run --rm -it \
    -u $(id -u) \
    -v "$PWD:/usr/src/tdesktop" \
    -e CONFIG=Debug \
    ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID=2040 \
    -D TDESKTOP_API_HASH=b18441a1ff607e10a989891a5462e627
```

Os binários finais ficam em `out/`. Use `strip` para reduzir o tamanho do binário final
antes de distribuir.

**IMPORTANTE sobre `TDESKTOP_API_ID`/`TDESKTOP_API_HASH`**: os valores `2040` /
`b18441a1ff607e10a989891a5462e627` são credenciais de teste **compartilhadas e com
rate limit** (usadas em CI/exemplos). Para builds pessoais/produção, o correto é o
usuário obter suas próprias credenciais em `https://my.telegram.org` e usar
`-D TDESKTOP_API_ID=SEU_ID -D TDESKTOP_API_HASH=SEU_HASH`. Nunca use
`-DTDESKTOP_API_TEST=ON` em build "de uso real" — isso força as credenciais de teste
rate-limited (ponto real relatado por mantenedores de pacotes AUR).

### 2.4. Integração com VS Code (Dev Containers)

Depois do passo 2.2, abra a pasta no VS Code, instale a extensão **Dev Containers**, e
adicione a `.vscode/settings.json`:

```json
{
    "cmake.configureSettings": {
        "TDESKTOP_API_ID": "SEU_API_ID",
        "TDESKTOP_API_HASH": "SEU_API_HASH"
    }
}
```

Depois use **Reopen in Container** (botão verde no canto inferior esquerdo). Esse é o
fluxo mais confortável para iteração/desenvolvimento de features, com IntelliSense
funcionando corretamente dentro do container.

## 3. Build "nativo" (fora do Docker) — para iteração rápida

Não é o fluxo oficialmente documentado em detalhe pelo projeto, mas é comum entre
mantenedores de pacotes (AUR, Nix, etc.) compilar diretamente com CMake + Ninja no
sistema, desde que as dependências (Qt6, OpenSSL, etc.) estejam resolvidas via
gerenciador de pacotes da distro ou via os mesmos submódulos. Padrão geral usado por
pacotes AUR:

```bash
cmake -B build -S Telegram \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DTDESKTOP_API_ID=SEU_API_ID \
    -DTDESKTOP_API_HASH=SEU_API_HASH \
    -DDESKTOP_APP_USE_PACKAGED_FONTS=OFF \
    -Wno-dev
cmake --build build --parallel $(($(nproc)/2))
```

Pontos de atenção:
- **Compilador**: precisa de suporte C++17/20 completo — GCC muito antigo falha em
  `CMake/TdSetUpCompiler.cmake` com `No C++17 support in the compiler`. Recomende
  GCC 12+/Clang 15+ atualizados.
- Algumas configurações de pacote exigem `lld` como linker para builds otimizados —
  se o link falhar com erros estranhos de linker, sugira `-DCMAKE_LINKER=lld` ou
  instalar o pacote `lld` da distro.
- Se o projeto usa `tde2e`/TDLib como submódulo separado (chamadas E2E de voz/vídeo),
  pode ser necessário compilar e instalar essa dependência antes, apontando
  `-Dtde2e_DIR=.../install/lib/cmake/tde2e` ao configurar o CMake principal — este é
  um padrão visto em pacotes empacotados (AUR); confirme se ainda se aplica na versão
  atual do repo antes de repassar como certo.

## 4. Empacotamento para distribuição em Linux

- **AUR (Arch)**: pacotes `ayugram-desktop` (build from source) e
  `ayugram-desktop-bin` (binário pré-compilado) mantidos pela comunidade.
- **Flatpak**: repositório comunitário dedicado
  (`github.com/0FL01/AyuGramDesktop-flatpak`), não gerenciado pela equipe principal.
- **RPM**: disponível via RPM Fusion em algumas distros.
- **AppImage/genérico**: possível gerar manualmente a partir do binário Linux com as
  ferramentas usuais (`linuxdeploy`, etc.) — não há pipeline oficial documentado do
  projeto para isso; trate como tarefa de packaging custom.

## 5. Tabela de troubleshooting (erros mais comuns)

| Sintoma | Causa provável | Solução |
|---|---|---|
| `No C++17 support in the compiler` (`TdSetUpCompiler.cmake`) | Compilador GCC/Clang desatualizado | Atualize toolchain (GCC 12+/Clang 15+) |
| Máquina trava/congela durante o build | Ninja usa todos os núcleos, RAM insuficiente | Limite jobs: `cmake --build build --parallel N` ou `export CMAKE_BUILD_PARALLEL_LEVEL=N`; adicione swap |
| Clone de submódulos extremamente lento | Muitos submódulos em série | `export GIT_CONFIG_PARAMETERS="'submodule.fetchJobs=4'"` |
| `error while loading shared libraries: libabsl_strings.so...` ao rodar o binário | Dependências do sistema (abseil, etc.) desatualizadas em relação ao binário instalado | Reconstrua o pacote/binário após atualizar bibliotecas do sistema (comum após updates gerais da distro) |
| `pull access denied for tdesktop` no `docker run` | Nome de imagem local incorreto / imagem não baixada | Use exatamente `ghcr.io/telegramdesktop/tdesktop/centos_env:latest` (não construir tag local `tdesktop:centos_env`) |
| Credenciais de API "bloqueadas"/rate limited em uso normal | `-DTDESKTOP_API_TEST=ON` ou uso das credenciais de exemplo (`2040`/`b18441...`) em produção | Obter API ID/Hash próprios em `my.telegram.org` |
| Link falha com "undefined reference" após adicionar novo `.cpp` | Arquivo novo não foi incluído nas listas de fontes do `Telegram/CMakeLists.txt` | Adicionar o arquivo à lista correta de fontes e reconfigurar o CMake |
| Erro de PDB/debug info em builds otimizados (mais comum no Windows, mas relevante ao comparar plataformas) | Configuração de debug info incompatível | Não se aplica diretamente ao Linux; ignore, é problema documentado só no build Windows/MSVC |

## 6. Checklist rápido antes de entregar um patch para o usuário compilar

1. O novo código compila isoladamente (sem warnings novos com `-W4`/`-Wall -Wextra`)?
2. Todo arquivo novo foi adicionado ao CMake?
3. Strings novas de UI foram adicionadas ao sistema de localização (não deixe texto
   hardcoded fora do sistema `lang_auto`/`.strings`)?
4. A feature usa `rpl::` corretamente para reatividade, sem vazar conexões?
5. Testado ao menos um build local (Debug) antes de sugerir o build final de release?
