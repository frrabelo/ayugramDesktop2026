# Bibliotecas Necessárias para Rodar o AyuGram no Linux

Como o AyuGram é compilado usando um ambiente baseado em CentOS (Rocky Linux) com a maioria das dependências linkadas de forma estática (incluindo Qt, FFmpeg e OpenSSL), ele possui uma compatibilidade muito alta com diversas distribuições Linux. 

No entanto, por ser um binário dinâmico, ele ainda depende de bibliotecas básicas do sistema para som, interface gráfica (X11/Wayland), fontes e comunicação com o sistema.

Abaixo estão as dependências exigidas e as instruções para instalá-las nas principais distribuições Linux.

## Distribuições Baseadas em Debian / Ubuntu (Ubuntu, Debian, Linux Mint, Pop!_OS)

Execute o seguinte comando no terminal para instalar todas as dependências de runtime necessárias:

```bash
sudo apt update
sudo apt install -y \
  libx11-6 \
  libx11-xcb1 \
  libxcomposite1 \
  libxcursor1 \
  libxdamage1 \
  libxext6 \
  libxfixes3 \
  libxi6 \
  libxrandr2 \
  libxrender1 \
  libxtst6 \
  libxcb1 \
  libxcb-keysyms1 \
  libxcb-screensaver0 \
  libxcb-shape0 \
  libxcb-shm0 \
  libxcb-util1 \
  libxcb-image0 \
  libxcb-icccm4 \
  libxcb-sync1 \
  libxcb-xfixes0 \
  libxcb-randr0 \
  libxcb-render0 \
  libxcb-xkb1 \
  libxkbcommon0 \
  libxkbcommon-x11-0 \
  libfontconfig1 \
  libfreetype6 \
  libdbus-1-3 \
  libpulse0 \
  libasound2 \
  libglib2.0-0 \
  libgobject-2.0-0 \
  libgio-2.0-0 \
  libstdc++6 \
  libgcc-s1 \
  libgbm1 \
  libdrm2
```

## Distribuições Baseadas em Fedora / Red Hat (Fedora, RHEL, Rocky Linux)

Execute o seguinte comando no terminal:

```bash
sudo dnf install -y \
  libX11 \
  libXcomposite \
  libXcursor \
  libXdamage \
  libXext \
  libXfixes \
  libXi \
  libXrandr \
  libXrender \
  libXtst \
  libxcb \
  xcb-util-keysyms \
  xcb-util-image \
  xcb-util-wm \
  libxkbcommon \
  libxkbcommon-x11 \
  fontconfig \
  freetype \
  dbus-libs \
  pulseaudio-libs \
  alsa-lib \
  glib2 \
  libstdc++ \
  libgbm \
  libdrm
```

## Distribuições Baseadas em Arch Linux (Arch, Manjaro, EndeavourOS)

Execute o seguinte comando no terminal:

```bash
sudo pacman -S --needed \
  libx11 \
  libxcomposite \
  libxcursor \
  libxdamage \
  libxext \
  libxfixes \
  libxi \
  libxrandr \
  libxrender \
  libxtst \
  libxcb \
  xcb-util-keysyms \
  xcb-util-image \
  xcb-util-wm \
  libxkbcommon \
  libxkbcommon-x11 \
  fontconfig \
  freetype2 \
  dbus \
  libpulse \
  alsa-lib \
  glib2 \
  gcc-libs \
  mesa \
  libdrm
```
