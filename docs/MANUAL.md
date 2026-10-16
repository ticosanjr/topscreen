# Manual do usuário — TopScreen

O **TopScreen** é um plugin de **efeitos de captura de tela ao vivo** para o OBS.
Ele aplica cursor bonito, efeitos de clique, zoom suave, fundo com cantos
arredondados, overlay de teclas e mais — **ao vivo**, sem gravar e sem editar.

Ele **não grava** e **não gera arquivo**: entrega a imagem processada direto
para o OBS, como um filtro, para você usar numa cena.

---

## 1. Requisitos

- **OBS Studio 30.0+** (Windows ou Linux)
- Uma **Captura de Tela** (Display/Screen Capture) como fonte no OBS

> No Linux, para o overlay de teclas é preciso dar permissão de leitura em
> `/dev/input` (ver §9).

---

## 2. Instalação

**Linux**
```bash
./scripts/install-linux.sh            # plugin de usuário (sem sudo)
```

**Windows**
```powershell
pwsh -File scripts\install-windows.ps1                    # plugin de usuário
pwsh -File scripts\install-windows.ps1 -ObsDir D:\OBS     # OBS portable
```

Detalhes e opções avançadas em [`BUILD.md`](BUILD.md).

---

## 3. Primeiros passos

1. No OBS, adicione uma fonte **Captura de Tela** e escolha a tela.
2. Clique com o botão direito na fonte → **Filtros** → **+** → **TopScreen FX**.
3. Ajuste em **Propriedades** (as abas/grupos estão descritos no §5).
4. Crie atalhos em **Configurações → Atalhos → TopScreen** (§6).
5. (Opcional) Escolha um **preset** pronto com o atalho *Próximo preset*.

> **Dica:** abra o dock **TopScreen** (menu **Exibir → Docks → TopScreen**) para
> ver o estado e trocar de preset rapidamente.

---

## 4. Conceitos

### Modo de cada recurso
Alguns recursos podem ficar **ligados**, **desligados** ou **por atalho**.
O **Efeitos habilitados** (topo das Propriedades) é o "mestre": desliga tudo de
uma vez (útil como push-to-talk de efeitos).

### Cursor próprio
Ao ativar os efeitos, o TopScreen **desliga o cursor nativo da captura** e
desenha o dele, suavizado e escalável. Por isso você vê **um só cursor**.

### Coordenadas
Os efeitos trabalham sobre a **área capturada**. O cursor é mapeado do mouse
para essa área (assumindo a captura no canto superior esquerdo da tela).

---

## 5. Referência de uso (por grupo das Propriedades)

### Efeitos habilitados
Liga/desliga **todos** os efeitos do filtro.

### Plano de fundo
Coloca a tela "flutuando" sobre um fundo, com cantos e sombra.

| Opção | O que faz |
|-------|-----------|
| **Ativar fundo** | Liga o fundo + cantos + espaçamento |
| **Estilo** | Cor sólida, gradiente vertical ou diagonal |
| **Cor A / Cor B** | Cores do fundo (B só aparece nos gradientes) |
| **Espaçamento (px)** | Margem entre a tela e a borda |
| **Cantos arredondados (px)** | Raio dos cantos da tela |
| **Sombra / Opacidade / Desfoque** | Sombra atrás da tela |

### Cursor
| Opção | O que faz |
|-------|-----------|
| **Mostrar cursor** | Desenha o cursor próprio (desliga o nativo) |
| **Estilo** | Um dos 7 cursores (ver abaixo) |
| **Tamanho** | Escala do cursor (1.0 = padrão) |
| **Suavização** | 0 = "flutuante", 1 = "colado" ao mouse |
| **Inclinação na curva (sway)** | O cursor inclina levemente ao virar |
| **Sombra do cursor** | Sombra para destacar do fundo |
| **Ocultar quando parado / Delay** | Some quando você para de mexer |
| **Voltar ao início quando parado / Tempo** | Cursor "volta" (vídeos em loop) |

**Estilos de cursor disponíveis (7)**

| Estilo | Aparência |
|--------|-----------|
| macOS (clássico) | Seta preta com contorno branco |
| Windows (clássico) | Seta branca com contorno preto |
| Breeze (KDE) | Seta do tema Breeze |
| Bibata (moderno) | Seta arredondada moderna (Bibata Modern Classic) |
| Adwaita | Seta do tema Adwaita (GNOME) |
| Google Dot | Círculo com ponto (estilo Google) |
| Mão (pointer) | Mão apontando |

> Os bitmaps são gerados por `tools/make-cursor-atlas.py` (lê temas de cursor
> instalados em `/usr/share/icons`) e ficam em `data/cursors/`.

### Clique esquerdo / Clique direito
Cada botão tem seu próprio efeito, independente.

| Opção | O que faz |
|-------|-----------|
| **Efeito** | Onda (ripple), Orbe, Anel ou Força |
| **Tamanho / Cor / Duração / Curva** | Aparência e tempo do efeito |

### Som de clique
| Opção | O que faz |
|-------|-----------|
| **Ativar som** | Toca um clique curto |
| **Volume** | Volume do som |
| **No clique esquerdo / direito** | Em quais botões tocar |

> O som toca no **áudio do sistema** (entra no OBS pela sua captura de Áudio do
> Desktop). Ainda não sai pelo canal de áudio da própria fonte.

### Zoom
| Opção | O que faz |
|-------|-----------|
| **Modo** | `Desligado`, `Automático` (segue cliques/cursor) ou `Manual` |
| **Intensidade** | Quanto amplia (1.0 = sem zoom) |
| **Velocidade (ms)** | Duração da animação |
| **Tempo sem foco (ms)** | Após parar, volta ao normal |
| **Intensidade do pan** | Quanto a tela "acompanha" o mouse |

### Movimento (motion blur)
| Opção | O que faz |
|-------|-----------|
| **Desfoque de movimento** | Borrão radial durante o zoom |
| **Intensidade** | Força do borrão |

### Spotlight
| Opção | O que faz |
|-------|-----------|
| **Ativar spotlight** | Escurece tudo menos um círculo que segue o cursor |
| **Raio** | Tamanho do círculo |
| **Escurecimento** | Quanto escurece fora do círculo |

### Teclas
| Opção | O que faz |
|-------|-----------|
| **Mostrar teclas digitadas** | Mostra o que você digita na base da tela |
| **Somente modificadores** | Mostra só Ctrl/Shift/Alt/Win |
| **Tempo na tela (ms)** | Quanto tempo cada tecla fica |
| **Máximo de teclas** | Quantas teclas simultâneas mostrar |

### Marca-texto
| Opção | O que faz |
|-------|-----------|
| **Ativar marca-texto** | Destaca a região que você arrasta com o botão esquerdo |
| **Cor** | Cor do destaque (use alfa < 255 para translúcido) |
| **Tempo na tela (ms)** | Quanto tempo fica após soltar |
| **Desvanecer (ms)** | Duração do fade |

---

## 6. Atalhos

Em **Configurações → Atalhos → TopScreen**:

| Ação | Efeito |
|------|--------|
| **Ativar/desativar efeitos** | Liga/desliga tudo (mestre) |
| **Ativar/desativar fundo** | Liga/desliga só o fundo/cantos |
| **Zoom in / Zoom out / Resetar zoom** | Zoom manual |
| **Próximo preset / Preset anterior** | Troca de preset |

**Linux/Wayland:** atalhos globais do OBS podem não funcionar fora do X11;
use os atalhos com o OBS em foco, ou o **obs-websocket** para disparar ações.

---

## 7. Presets

Presets prontos (arquivos em `data/presets/`):

| Preset | Cara |
|--------|------|
| **Demo** | Gradiente, cursor destacado, zoom automático |
| **Tutorial** | Fundo sólido, som de clique, cursor suave |
| **Apresentacao** | Fundo escuro, sem zoom, sem som |

Troque com o atalho *Próximo/Anterior preset* (ou pelo dock). Para criar o seu,
copie um arquivo e ajuste — o formato está em [`CONFIGURACAO.md`](CONFIGURACAO.md).

---

## 8. Receitas

**Demo de produto (parecido com "Screen Studio")**
- Fundo = Gradiente diagonal, Espaçamento 64, Cantos 18, Sombra ligada
- Cursor = Tamanho 1.6, Suavização 0.8, sway 0.4
- Zoom = Automático, Intensidade 1.6
- Clique esquerdo = Onda; Direito = Orbe

**Tutorial / aula**
- Fundo = Cor sólida, Cantos 12
- Teclas = ligado, Tempo 2000 ms
- Som de clique = ligado (volume 0.5)
- Zoom = Automático, Intensidade 1.4

**Apresentação / reunião**
- Fundo = Cor escura, Espaçamento 80
- Zoom = Desligado
- Spotlight = disponível por atalho (quando quiser destacar)

**Live coding**
- Teclas = ligado, Máximo 6
- Zoom = Automático leve (1.3)
- Marca-texto = ligado para destacar trechos de código

---

## 9. Linux: permissão para o overlay de teclas

O overlay lê as teclas de `/dev/input` (evdev), o que funciona em X11 **e**
Wayland. Exige permissão:

```bash
sudo cp data/udev/99-topscreen.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# relogar a sessão ajuda
```

Sem isso, o plugin avisa no log (`evdev indisponivel`) e o overlay não aparece.
Para testar mesmo assim:

```bash
TOPSCREEN_DEBUG_KEY=Ctrl obs
```

---

## 10. Desempenho

- Os efeitos rodam na **GPU**, no processo do OBS (sem gravação, sem cópia).
- Custo maior: **sombra** (desfoque) e **motion blur**. Se o PC for fraco:
  reduza o **Desfoque da sombra** ou desligue o **Movimento**.
- Resoluções altas + monitor de 60 Hz são tranquilos; monitores de 120/144 Hz
  custam mais.
- Em render por software (sem GPU, ex.: `llvmpipe`), tudo funciona, mas mais lento.

---

## 11. Solução de problemas

**O filtro não faz nada (passa direto)**
- Veja o log: `obs --verbose 2>&1 | grep -i topscreen`. Se aparecer
  `falha ao criar o efeito`, o arquivo `data/effects/topscreen.effect` não foi
  encontrado — reinstale.

**O plugin não aparece / "Duplicate library"**
- Você tem o plugin instalado em dois lugares. Fique com **um** só
  (usuário **ou** sistema). Veja [`BUILD.md`](BUILD.md).

**Vejo dois cursores**
- A captura estava com cursor nativo ligado. O TopScreen desliga
  `capture_cursor`/`show_cursor` automaticamente — se a sua fonte usar outro
  nome, desligue manualmente a opção de cursor da fonte.

**O cursor está atrasado**
- Aumente **Suavização** para 1.0 e reduza **Inclinação na curva** para 0.

**O cursor/mouse não bate com a tela**
- O mapeamento assume a captura no canto superior esquerdo da tela. Em
  multi-monitor com a tela secundária, o cursor pode deslocar.

**Sem teclas no overlay (Linux)**
- Aplique a regra udev do §9.

**O dock não aparece**
- O build não tinha Qt6/obs-frontend-api. Instale `qt6-base-dev` e reinstale.

**Zoom "pula" ou treme**
- Aumente **Velocidade (ms)** e reduza **Intensidade**.

---

## 12. FAQ

**Preciso gravar?**
Não. O TopScreen só entrega a imagem ao OBS. Quem grava/transmite é o OBS.

**Ele deixa editar depois?**
Não — por definição é só **ao vivo**. Para edição posterior use um editor.

**Funciona com "Captura de Janela" ou "Captura de Jogo"?**
Sim, o filtro funciona sobre qualquer fonte de vídeo. A seleção de tela é sua,
na fonte.

**Consigo usar dois filtros TopScreen ao mesmo tempo?**
Sim, mas cada um é independente; o dock controla o **primeiro** filtro ativo.

**Dá para desligar só o fundo num momento específico?**
Sim: atalho **Ativar/desativar fundo**.

**Windows: preciso de admin?**
Não, o instalador padrão usa a pasta de plugins do usuário.

---

## 13. Limitações atuais

- **Webcam bubble** e **zoom em região** ainda não foram implementados.
- O **som de clique** usa o áudio do sistema (não o canal de áudio da fonte).
- **Atalhos globais no Wayland** dependem do compositor (portal ainda não
  implementado).
- A **fonte de captura própria** (escolher a tela dentro do TopScreen) está
  planejada; hoje você usa a Captura de Tela do OBS.

Veja o que está pronto em [`TAREFAS.md`](TAREFAS.md) e como verificar em
[`VERIFICACAO.md`](VERIFICACAO.md).
