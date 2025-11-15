# OBS GameDetector – Plugin para OBS Studio

O **GameDetector** é um plugin para o **OBS Studio** que detecta automaticamente jogos em execução no computador e permite acionar automações, trocar cenas, enviar mensagens no chat da Twitch e muito mais.

Ele facilita a vida de streamers ao identificar o jogo sendo jogado e executar ações configuráveis de forma automática.

---

## Funcionalidades

### Detecção Automática de Jogos
- Monitoramento contínuo de processos do sistema.
- Suporte a lista **manual** e **automática** de jogos.
- Identificação de início, encerramento e mudança do jogo ativo.
- Processamento assíncrono para não travar o OBS.

---

### Lista Manual / Automática
O usuário pode:

- Adicionar jogos manualmente (Nome + executável).
- Remover jogos.
- Limpar a lista.
- Escanear automaticamente os jogos instalados.

Os jogos são salvos no sistema de configuração nativo do OBS.

---

### Integração com Twitch Chat
- Envio de mensagens automáticas quando um jogo inicia.
- Geração rápida de token via navegador.
- Armazenamento seguro do token no OBS.
- Permissões mínimas: `chat:read` e `chat:edit`.

---

### Integração com o OBS
O plugin emite sinais internos quando:

- Um jogo inicia.
- Um jogo fecha.
- O jogo ativo muda.

Esses eventos podem ser usados em:

- Scripts (Lua/Python)
- Automação de cenas
- Plugins externos
- Hotkeys

---

## Como Usar

### 1. Instalação
Descompacte o zip e copie as pastas `data` e `obs-plugins` na pasta:

```
C:\Program Files\obs-studio\
```

Reinicie o OBS.

---

### 2️. Abrindo o painel de configuração
No OBS:

**Menu → Ferramentas → Game Detector**

Aqui você pode:

- Editar a lista de jogos
- Escanear automaticamente
- Configurar token da Twitch
- Salvar alterações

---

### 3️. Configurando jogos
Na tabela você pode:

- ➕ Adicionar jogo  
- ➖ Remover jogo  
- 🧹 Limpar lista  
- 🔍 Escanear jogos instalados  

Cada entrada é composta por:

| Campo     | Descrição                   |
|-----------|------------------------------|
| Nome      | Nome amigável do jogo       |
| Executável | Nome do `.exe` do jogo      |

---

### 4️. Configurar mensagens automáticas na Twitch
1. Clique em **Gerar Token**
2. O navegador abre com permissões mínimas
3. Copie o token e cole no campo "Twitch Access Token"
4. Salve

---

## Configurações Salvas

O plugin armazena automaticamente:

- Token da Twitch  
- Lista manual de jogos  
- Último escaneamento  
- Preferências gerais  

Tudo salvo usando `obs_data_t`, integrado ao sistema de configs do OBS.

---

## Compatibilidade

- OBS Studio **29+**
- Windows **10/11**
- Compilado com:
  - OBS SDK
  - Qt 6
  - C++17

---

## Limitações

- Jogos devem ser identificáveis via executável.
- Renomear o `.exe` pode impedir a detecção.
- Jogos incomuns ou sem janela própria podem não ser detectados.
- Mensagens no chat exigem token válido.

---

## Contribuindo

Pull requests e sugestões são bem-vindas!

Você pode:

- Reportar bugs  
- Sugerir novas funções  
- Melhorar documentação  
- Criar testes  

---

## Licença

**GNU GENERAL PUBLIC LICENSE**

---