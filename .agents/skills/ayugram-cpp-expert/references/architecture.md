# Arquitetura do AyuGramDesktop — Referência Técnica

O AyuGramDesktop é um **fork direto** do `telegramdesktop/tdesktop` (o Telegram Desktop
oficial), mantendo ~95% da arquitetura original e adicionando uma camada de
funcionalidades extras. Isso significa: **a maior parte do conhecimento sobre a
arquitetura do tdesktop upstream se aplica diretamente ao AyuGram**, e vice-versa —
ao investigar algo, vale procurar em ambos os repositórios.

## 1. Estrutura de alto nível do repositório

```
AyuGramDesktop/
├── Telegram/
│   ├── SourceFiles/        # código-fonte principal do cliente (C++)
│   │   ├── ayu/            # (nome pode variar entre versões) — módulos específicos
│   │   │                   #   do fork: Ghost Mode, anti-recall, configurações extras,
│   │   │                   #   banco local ayudata.db
│   │   ├── api/             # camada de chamadas à API do Telegram (MTProto de alto nível)
│   │   ├── mtproto/         # implementação do protocolo MTProto
│   │   ├── data/            # modelos de dados (peers, mensagens, sessões, etc.)
│   │   ├── history/         # tela de conversa / histórico de mensagens
│   │   ├── settings/        # telas de configurações (é aqui que a maioria das
│   │   │                    #   novas opções de usuário deve ser exposta)
│   │   ├── boxes/           # diálogos modais ("boxes" no jargão do tdesktop)
│   │   ├── ui/               # widgets e componentes de interface específicos do app
│   │   ├── window/           # janelas principais, gerenciamento de sessões/contas
│   │   └── platform/linux/   # código específico de integração com Linux
│   │                         #   (D-Bus, notificações nativas, tray icon, etc.)
│   ├── lib_ui/               # biblioteca de UI reutilizável (parte do "desktop-app" framework)
│   ├── lib_base/             # utilitários de base (rpl::, base::, containers, etc.)
│   ├── lib_tl/               # geração/uso de esquemas TL (MTProto)
│   ├── lib_webview/          # embutido para renderizar conteúdo web (ex.: pagamentos, widgets)
│   ├── Resources/
│   │   └── langs/            # arquivos de tradução/localização (.strings)
│   ├── cmake/                 # módulos CMake auxiliares (opções, toolchain, etc.)
│   ├── build/
│   │   ├── prepare/           # scripts de preparo do ambiente (linux.sh, win.bat, mac.sh)
│   │   └── docker/            # Dockerfiles/scripts dos ambientes de build (centos_env)
│   └── CMakeLists.txt          # ponto de entrada do build do cliente
├── docs/                       # documentação de build (building-linux.md, etc.)
└── cmake/                      # (dependendo da versão) módulos CMake de nível raiz
```

⚠️ **Aviso importante**: a estrutura interna muda entre versões do `dev`. Sempre que
possível, confirme o caminho real no repositório (via ferramentas de leitura de código,
busca no GitHub, ou pedindo ao usuário para colar a árvore de diretórios relevante)
antes de afirmar "o arquivo está em X" com certeza absoluta.

## 2. Bibliotecas internas essenciais (compartilhadas com o tdesktop upstream)

Essas bibliotecas fazem parte do "desktop-app framework" mantido pela equipe do
Telegram Desktop e usado como base por vários forks:

- **`rpl::`** — biblioteca de programação reativa própria do projeto (não é RxCpp nem
  Qt signals/slots puro). Streams de valores/eventos são compostos com operadores como
  `rpl::map`, `rpl::filter`, `rpl::start_with_next`, etc. Qualquer UI nova que reaja a
  mudanças de estado (ex.: toggle de uma opção do Ghost Mode) deve ser implementada com
  `rpl::` para seguir o padrão do projeto, e não com callbacks soltos ou sinais Qt
  manuais quando já existe uma stream `rpl::` equivalente disponível.
- **`base::`** — utilitários de baixo nível: containers customizados
  (`base::flat_map`, `base::flat_set`), `base::weak_ptr`, `base::Timer`, algoritmos.
- **`style::`** — sistema de estilos declarativo próprio (arquivos `.style` e
  `.palette`), compilado em tempo de build para structs C++ tipados. **Não existe CSS
  livre**: cores, margens, fontes de um novo widget devem ser declaradas em arquivos
  `.style`/`.palette` e referenciadas via `style::` no C++, seguindo exatamente o
  padrão dos widgets vizinhos já existentes.
- **`lang_auto` / sistema de localização** — toda string visível ao usuário deve vir
  do sistema de tradução (`tr::lng_algo_nome()` ou equivalente gerado), nunca
  hardcoded em C++, para manter consistência com o resto do app e permitir tradução
  pela comunidade.

## 3. Onde vivem as funcionalidades específicas do AyuGram

Funcionalidades "de marca" do fork, para servir de referência de padrão ao implementar
algo novo semelhante:

- **Ghost Mode**: conjunto de toggles que alteram comportamento local de rede/UI
  (esconder "visto por último", suprimir indicador de digitação, enviar sem marcar
  como lido, enviar mensagens/reações "sem som"). Implementado como flags de
  configuração local que interceptam pontos específicos da camada de API antes de
  disparar as chamadas correspondentes ao servidor — a ideia central é que o
  comportamento de rede continue parecendo um cliente oficial (daí a ausência de
  padrão de banimentos associado a essas features).
- **Anti-recall / anti-delete / histórico de edições**: mantém uma cópia local
  (em um banco SQLite próprio, tipicamente `ayudata.db`, dentro da pasta de dados do
  usuário) das mensagens antes de serem apagadas/editadas pelo remetente, permitindo
  exibi-las mesmo após a exclusão/edição no servidor. Qualquer nova feature de
  "histórico local" deve seguir esse mesmo padrão de armazenamento local, sem nunca
  reenviar esse conteúdo para servidores de terceiros — é uma feature 100% local.
- **Local Premium (desbloqueio cosmético)**: desbloqueia elementos visuais que dependem
  apenas de flags client-side (emojis animados, algumas reações, temas), sem tocar em
  perks que dependem do servidor (ex.: limite de upload de 4 GB), já que estes últimos
  são impostos pelo backend do Telegram e não podem ser simulados no cliente.
- **Streamer Mode / customização de temas / tradutor embutido**: seguem o padrão comum
  de "settings toggle" + tela em `settings/` + (quando aplicável) integração com
  serviço externo (tradutor) via `lib_webview` ou chamada HTTP direta, isolada do
  fluxo MTProto principal.

Use esses exemplos como referência de "onde" e "como" plugar uma feature nova: a
resposta quase sempre é (1) uma flag em uma classe de configurações persistente,
(2) um toggle na tela de `settings/`, (3) um ponto de interceptação na camada
`api/`/`mtproto/` ou em `history/` conforme o efeito desejado.

## 4. Estilo de código C++ do projeto (tdesktop / AyuGram)

- **Padrão**: C++20, RAII estrito, uso extensivo de `base::weak_ptr` para evitar
  dangling pointers em callbacks assíncronos (comum em UI + rede assíncrona).
- **Nomenclatura**: `PascalCase` para classes/tipos e a maioria das funções membro
  públicas; `camelCase` para variáveis locais; membros privados geralmente com sufixo
  `_` (ex.: `_widget`, `_value`).
- **Sem exceções em código "hot path"**: o projeto historicamente evita exceções C++
  para controle de fluxo em código de rede/UI; prefira `std::optional`, códigos de
  erro/retorno ou `Expected`-like patterns já usados no código vizinho.
- **Sem herança múltipla desnecessária**: prefira composição + `rpl::` para
  comunicação entre componentes, seguindo o estilo predominante do projeto.
- **Warnings tratados como sinal real**: o projeto compila com `/W4` (MSVC) e
  `-Wall -Wextra` equivalentes em GCC/Clang; corrija warnings novos introduzidos por
  um patch antes de considerá-lo pronto.
- **Testes**: o projeto não tem uma suíte de testes automatizados extensa como um
  projeto greenfield — a validação é majoritariamente manual (build + uso interativo)
  e via CI de build multiplataforma. Ao propor uma feature, sugira um roteiro manual
  de teste claro (passos no app) em vez de assumir que existe cobertura automatizada.

## 5. Fluxo típico para adicionar uma nova opção de "Settings" (exemplo genérico)

1. Adicionar o campo de configuração persistente (ex.: um `bool` em uma struct/JSON de
   configurações do AyuGram, salvo em disco).
2. Expor o valor via um `rpl::variable<bool>` (ou padrão equivalente já usado no
   arquivo de settings vizinho) para que a UI reaja a mudanças automaticamente.
3. Criar/editar a seção correspondente em `settings/` com um `Ui::SettingsButton`/
   toggle seguindo o padrão visual (`style::`) já existente na tela.
4. Conectar o toggle da UI ao `rpl::variable` via `rpl::start_with_next` ou padrão
   equivalente, persistindo a mudança.
5. No ponto do código onde o comportamento efetivamente muda (ex.: antes de montar uma
   requisição MTProto, ou em `history/` ao renderizar uma mensagem), checar a flag e
   ramificar o comportamento.
6. Adicionar strings de UI ao sistema de localização.
7. Atualizar `CMakeLists.txt` se houver arquivos novos.
8. Compilar (ver `references/build-linux.md`) e testar manualmente o fluxo.
