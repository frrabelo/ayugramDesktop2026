# Planejamento de Alterações - Sistema de Log e Barra de Status

Este documento descreve o planejamento detalhado das alterações realizadas no AyuGram Desktop para implementar o sistema de logs e a barra de status de processos.

## Arquitetura das Alterações

As alterações foram divididas em três componentes principais:
1. **Sistema de Logging (`AyuLogger`)**: Responsável por persistir informações de execução e falhas em arquivos locais e gerenciar rotação.
2. **Interface Visual (`AyuStatusBar`)**: Componente gráfico herdando de `Ui::RpWidget` posicionado no topo da tela de chat para exibir o status atual, progresso e logs recentes.
3. **Integração com o Layout (`HistoryWidget`)**: Atualização do gerenciador de chat do Telegram Desktop para incluir, posicionar e gerenciar a visibilidade da barra de status.

---

## Checklist de Tarefas e Progresso

### 1. Sistema de Logs e Depuração (AyuLogger)
- [x] **Definição de Interface (`ayu_logger.h`)**:
  - Declaração de funções para inicialização, gravação de logs normais/erros e abertura da pasta de logs.
- [x] **Implementação do Gravador (`ayu_logger.cpp`)**:
  - Criação da pasta de logs `ayu_logs` no diretório de dados locais da aplicação.
  - Implementação de escrita thread-safe usando `QMutexLocker`.
  - Adição de suporte à rotação de logs automática quando o arquivo exceder 10MB.
  - Integração da função `openLogsFolder()` para abrir a pasta nativa do sistema operacional através de `QDesktopServices`.

### 2. Barra de Status de Processos (AyuStatusBar)
- [x] **Definição de Componente Visual (`ayu_status_bar.h`)**:
  - Declaração da classe `AyuStatusBar` como subclasse de `Ui::RpWidget`.
  - Definição de estados de expansão (`_expanded`) e variáveis de rastreamento de progresso.
- [x] **Desenho e Visualização (`ayu_status_bar.cpp`)**:
  - Estilização com estética escura e translúcida (Glassmorphic) usando `paintEvent`.
  - Exibição dinâmica das informações de progresso (`atual/total`) e descrição da tarefa corrente.
  - Renderização de linhas de log recentes quando o painel for expandido.
  - Desenho de botões interativos para "LOGS" e setas de chevrons verticais de expansão.
- [x] **Eventos de Interação (`ayu_status_bar.cpp`)**:
  - Manipulação de cliques no botão de expandir para animar o tamanho da barra (`mousePressEvent`).
  - Redirecionamento de cliques no botão "LOGS" para chamar `AyuLogger::openLogsFolder()`.

### 3. Integração com a Tela de Chat (HistoryWidget)
- [x] **Gerenciamento do Ciclo de Vida (`history_widget.h`)**:
  - Inclusão do ponteiro `_ayuStatusBar` como filho de `HistoryWidget`.
- [x] **Posicionamento no Layout (`history_widget.cpp`)**:
  - Ajuste de margens na função `updateControlsGeometry()` para que a barra de status fique logo no topo da tela de chat, deslocando a lista de mensagens verticalmente sem sobrepor outros componentes.
- [x] **Visibilidade Condicional (`history_widget.cpp` -> `updateAyuStatusBar()`)**:
  - Exibição automática da barra apenas quando processos de encaminhamento estiverem ativos para o canal/grupo atual.
