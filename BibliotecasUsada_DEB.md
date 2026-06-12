# Dependências do Pacote DEB (AyuGram) para ZorinOS 18

Ao instalar o pacote nativo `.deb` (`ayugram-desktop-zorinos18-amd64.deb`) no ZorinOS 18, o gerenciador de pacotes do sistema (`apt` ou a loja de aplicativos do Zorin) resolverá e instalará automaticamente todas as bibliotecas necessárias.

Caso você precise instalar manualmente as dependências de runtime (por exemplo, ao extrair o binário de dentro do pacote), execute o comando abaixo.

## Comando de Instalação Rápida no ZorinOS 18 / Ubuntu

Abra o terminal e execute:

```bash
sudo apt update
sudo apt install -y \
  libc6 \
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

## Resumo das Bibliotecas e Suas Funções

| Biblioteca | Pacotes ZorinOS 18 / Debian / Ubuntu | Finalidade no AyuGram |
| --- | --- | --- |
| **Glibc & GCC Runtime** | `libc6`, `libstdc++6`, `libgcc-s1` | Execução básica de código compilado em C++17/20. |
| **X11 / Display Server** | `libx11-6`, `libx11-xcb1`, `libxcomposite1`, `libxcursor1`, `libxdamage1`, `libxext6`, `libxfixes3`, `libxi6`, `libxrandr2`, `libxrender1`, `libxtst6` | Protocolo X11 básico para renderização da interface e captura de cliques/teclado. |
| **XCB (X C Binding)** | `libxcb1`, `libxcb-keysyms1`, `libxcb-screensaver0`, `libxcb-shape0`, `libxcb-shm0`, `libxcb-util1`, `libxcb-image0`, `libxcb-icccm4`, `libxcb-sync1`, `libxcb-xfixes0`, `libxcb-randr0`, `libxcb-render0`, `libxcb-xkb1` | Comunicação direta e eficiente com o X Server. |
| **Teclado & Entrada** | `libxkbcommon0`, `libxkbcommon-x11-0` | Mapeamento de layouts de teclado modernos da interface Qt. |
| **Fontes & Texto** | `libfontconfig1`, `libfreetype6` | Localização e renderização das fontes do sistema e emojis. |
| **Som & Chamadas** | `libpulse0`, `libasound2` | Backend ALSA e PulseAudio para reprodução de áudio e chamadas de voz de alta fidelidade. |
| **Barramento de Mensagens** | `libdbus-1-3` | Integração de notificações do sistema, tray icon e portal do desktop. |
| **Aceleração Gráfica** | `libgbm1`, `libdrm2` | Renderização por hardware (OpenGL/Mesa) no X11 e Wayland. |
| **Glib / GIO** | `libglib2.0-0`, `libgobject-2.0-0`, `libgio-2.0-0` | Manipulação de eventos básicos de sistema e chamadas a portas do ecossistema GNOME do ZorinOS. |
