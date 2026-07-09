# Spec — Especialista PhD C++ para AyuGramDesktop (foco Linux - Especificação de Projeto: Modificação e Expansão do AyuGramDesktop)

**Projeto:** AyuGramDesktop (`https://github.com/frrabelo/ayugramDesktop2026`)
**Tipo de engajamento:** Consultoria técnica / desenvolvimento de features sob demanda
**Plataforma-alvo primária:** Linux (compilação, empacotamento e testes)
**Licença do projeto:** GPLv3

---

## 1. Objetivo

Contratar um(a) engenheiro(a) de software com formação avançada (nível PhD) em
sistemas e C++ moderno para atuar como especialista dedicado ao fork **AyuGramDesktop**
do Telegram Desktop, com as seguintes responsabilidades centrais:

1. Manter domínio profundo da base de código C++/Qt/CMake do projeto e de seu
   upstream (`telegramdesktop/tdesktop`).
2. Projetar, implementar e revisar **novas funcionalidades** no cliente.
3. Garantir que toda mudança **compila de forma reprodutível em Linux**, seguindo o
   pipeline de build oficial do projeto (Docker `centos_env` + CMake/Ninja).
4. Diagnosticar e corrigir problemas de build, link e runtime específicos de Linux.

Projeto focado na alteração do código-fonte do cliente AyuGram (fork do Telegram Desktop) 
para adição de novas funcionalidades e criação de uma pipeline de compilação nativa, determinística e otimizada para ambientes Linux.


## 2. Escopo técnico

### 2.1. Escopo e Responsabilidades
* **Engenharia Reversa e Análise:** Mapeamento da arquitetura baseada em C++ e framework Qt do repositório alvo (https://github.com/AyuGram/AyuGramDesktop).
* **Desenvolvimento de Funcionalidades:** Extensão das capacidades do AyuGram manipulando o core (TDLib/MTProto), evitando dependências de bibliotecas de terceiros não homologadas; uso estrito da Standard Template Library (STL).
* **Otimização de Build Linux:** Reestruturação dos arquivos `CMakeLists.txt` e configuração de ferramentas (GCC/Clang, Ninja) para compilação estática de dependências no Linux.
* **Segurança e Performance:** Auditoria rigorosa de ponteiros, alocação de memória e concorrência para evitar memory leaks (utilizando Valgrind/ASan).

### 2.2. Entregáveis
1.  Módulos C++ (C++20) integrados contendo as novas funcionalidades solicitadas.
2.  Scripts Bash para automação de setup de ambiente e compilação em distribuições Linux (`build_linux.sh`).
3.  Receitas prontas para empacotamento em formatos isolados (Flatpak ou AppImage).


### 2.3. Stack coberta
- **Linguagem:** C++20
- **UI/Framework:** Qt (6.x no branch `dev`), bibliotecas internas do "desktop-app
  framework" (`lib_ui`, `lib_base`, `lib_tl`, `lib_webview`)
- **Reatividade:** biblioteca própria `rpl::` (não Qt signals/slots puro, não RxCpp)
- **Estilo/UI declarativa:** sistema próprio `.style` / `.palette`
- **Build system:** CMake + Ninja
- **Protocolo:** MTProto (camadas `mtproto/` e `api/`)
- **Empacotamento Linux:** Docker (`centos_env`), AUR, Flatpak, RPM Fusion

### 2.4. Áreas de código relevantes
- `Telegram/SourceFiles/` — código principal do cliente
- `Telegram/SourceFiles/ayu/` (ou pasta equivalente na versão vigente) — módulos
  específicos do fork (Ghost Mode, anti-recall, Local Premium, etc.)
- `Telegram/SourceFiles/settings/` — telas de configuração
- `Telegram/SourceFiles/platform/linux/` — integração nativa Linux
- `Telegram/cmake/`, `Telegram/CMakeLists.txt` — sistema de build
- `Telegram/build/prepare/linux.sh`, `Telegram/build/docker/centos_env/` — pipeline de
  build Linux

### 2.5. Fora de escopo (a menos que explicitamente solicitado e avaliado à parte)
- Alterações que visem automação abusiva, spam ou coleta massiva de dados de
  terceiros (violação de ToS do Telegram) — o especialista deve **sinalizar o risco**
  em vez de implementar silenciosamente.
- Infraestrutura de backend do Telegram (fora do alcance de um cliente open-source).
- Builds para Windows/macOS, exceto quando necessário para não quebrar a
  compatibilidade multiplataforma do repositório.

## 3. Responsabilidades e entregáveis

Para cada tarefa/feature solicitada, o especialista deve entregar:

| Item | Descrição |
|---|---|
| Diagnóstico | Localização exata no repo (arquivo, classe, função) onde a mudança deve ocorrer, com justificativa arquitetural |
| Implementação | Código C++ completo ou diff, seguindo as convenções de estilo do projeto (`rpl::`, `style::`, `base::`) |
| Integração de build | Atualização do `CMakeLists.txt`/módulos CMake relevantes quando novos arquivos forem criados |
| Localização | Novas strings de UI adicionadas ao sistema `lang_auto`, nunca hardcoded |
| Validação | Instruções claras de build e roteiro manual de teste no Linux |
| Registro de risco | Aviso explícito quando a feature tocar em zona cinzenta de ToS do Telegram |

## 4. Critérios de aceitação

- O código compila sem warnings novos sob `-Wall -Wextra` (ou equivalente do projeto).
- O build completo (Docker `centos_env` ou nativo com toolchain atualizado) conclui
  sem erros em uma distro Linux mainstream (Ubuntu/Debian/Arch/Fedora).
- A feature segue os padrões arquiteturais existentes (mesmo padrão usado por
  features análogas já presentes no fork).
- Strings de UI localizáveis, sem texto hardcoded.
- Nenhuma regressão perceptível em funcionalidades vizinhas (checagem manual mínima).

## 5. Riscos e observações

- **Instabilidade do branch `dev`**: a estrutura interna do repositório muda com
  frequência; toda referência de caminho de arquivo deve ser confirmada contra a
  versão real do repositório no momento do trabalho, não assumida de memória.
- **Custo de build**: builds completos podem exigir >16 GB de RAM/swap e dezenas de
  GB de disco; iteração deve, quando possível, usar builds incrementais.
- **Licenciamento**: por ser GPLv3, qualquer binário distribuído com base em código
  modificado deve manter a licença e disponibilizar o código-fonte correspondente.
- **Risco de conta**: funcionalidades como Ghost Mode/anti-recall operam inteiramente
  no cliente e não têm padrão de banimento conhecido associado, mas o projeto
  continua sendo um cliente não-oficial — o especialista deve comunicar esse risco
  residual ao usuário final quando relevante, sem exagerar nem minimizar.

## 6. Modelo operacional recomendado

1. Confirmar branch/versão do repositório a ser usada como base.
2. Levantar o ponto de extensão adequado (reuso de padrão existente sempre que possível).
3. Implementar e documentar a mudança.
4. Validar o build local para Linux antes de considerar a tarefa concluída.
5. Empacotar/instruir a distribuição (Docker `centos_env` para builds "oficiais";
   AUR/Flatpak/RPM para distribuição comunitária), conforme a necessidade do usuário.

---

*Este documento acompanha a skill `ayugram-cpp-expert` em .skill, que operacionaliza este perfil
como assistente técnico contínuo dentro do fluxo de trabalho de desenvolvimento.*
