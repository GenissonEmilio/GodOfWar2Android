# Desempenho no Linux — 0.1.1

A versão 0.1.0 deixava a escolha da GPU com o sistema. Neste notebook híbrido,
isso selecionava a Intel integrada, mesmo com uma RTX 3050 disponível.
A versão 0.1.1 escolhe a NVIDIA secundária com driver proprietário ativo,
valida a abertura de OpenGL num processo oculto e retorna à GPU do sistema se
esse teste falhar ou ultrapassar 12 segundos. A seleção afeta apenas o jogo.
Não altera drivers, serviços, perfil de energia ou configurações globais.

Implementação do offload baseada na [documentação oficial NVIDIA PRIME](https://download.nvidia.com/XFree86/Linux-x86_64/580.178.04/README/primerenderoffload.html).
Não é necessário instalar `nvidia-smi`, `glxinfo` ou `switcherooctl` para usar o launcher.
Opções PRIME externas definidas pelo usuário têm prioridade sobre a seleção automática.

## Configurações

Em **Gráficos → Desempenho**:

- GPU automática: prefere NVIDIA secundária quando disponível e validada.
- GPU padrão do sistema: mantém a escolha normal da sessão Linux.
- Resolução interna automática: 2× com offload NVIDIA; 1× nas demais configurações.
- Nativa 1×, Melhorada 2× ou Alta 3×: escolha manual da qualidade/custo de renderização.
- Mostrar contador de FPS ao iniciar: também pode ser alternado com F10 no jogo.

Reduzir a resolução interna não muda a resolução física da tela cheia.
Filtros e shaders continuam disponíveis. Preferências ficam em `performance.json`,
separadas da configuração de tela cheia, dos cartões de memória e do cache.
O runtime, o código recompilado e o identificador do compilador permanecem os mesmos
nesta atualização: `b2a91af7fdd6cdc4b6d1446e`.

## Método de medição local

22/10/2026, Fedora 44, i5-13420H, Intel UHD Raptor Lake-P e
NVIDIA RTX 3050 Laptop 6 GB. Mesa 26.2.4; driver NVIDIA 580.178.04.
Tela cheia 1920×1080, proporção 16:9, resolução interna 2×, sem shader de pós-processamento.
Executável, arquivos do disco, compilação e sequência de comandos iguais.
Três execuções consecutivas de 240 segundos; diretórios de teste e cartões separados
para não alterar o progresso do jogador. Sem compilação concorrente durante a medição.

Contador do jogo na entrada de `0x14E380`:
`GOW_PERF_DIAG=frames GOW_PERF_FRAME_PC=0x14e380`.
Esse modo apenas conta quadros; não habilita a instrumentação dos tempos de CPU.
Capturas a cada 20 segundos e `GOW_VU1N_DIAG=1` em ambas as execuções.

Sequência usada em `GOW2_PAD_SCRIPT`:

```text
5:start,10:x,13:x,16:x,30:start,31:x,40:start,41:x,60:start,61:x,90-230/0.7:sq,100-230/2:x,110-115:lup,130-136:lright,170-175:o
```

Os resultados de combate usam os relatórios de aproximadamente 5 segundos com
`t` entre 180 e 230 segundos. A média é a média dessas janelas; o mínimo é a
menor média de uma janela, não o mínimo de um quadro nem o indicador 1% low.
Menus e a cinemática de abertura ficam fora desse intervalo.
O script não fixa a simulação entre GPUs: há variação nos golpes e posição dos inimigos.
As capturas devem ser inspecionadas para confirmar que ambas as execuções estão em combate.

## Resultados

| Configuração | Média do jogo | Menor janela | Média da apresentação |
|---|---:|---:|---:|
| 0.1.0 · Intel · 2× | 36,33 FPS | 32,71 FPS | 41,45 FPS |
| 0.1.1 · NVIDIA automática · 2× | 48,59 FPS | 43,80 FPS | 59,17 FPS |
| 0.1.1 · NVIDIA automática · 1× | 47,46 FPS | 36,80 FPS | 59,25 FPS |

O ganho observado em 2× foi **33,7%** na média do jogo. Reduzir para 1× não
trouxe melhora nesta sequência; por isso o automático mantém 2× na NVIDIA.
A variação do combate entre execuções limita comparações pequenas entre 1×/2×.
O contador de apresentação próximo de 60 não significa 60 quadros novos do jogo:
ambos foram medidos separadamente. Ainda há oscilações e **não há 60 FPS fixos**.

As capturas de combate em 210 segundos foram inspecionadas: combate em Rhodes,
Kratos, cenário, efeitos e HUD renderizados em ambas as GPUs. As três execuções
encerraram normalmente, com texturas compartilhadas na GPU, sem `missing-target`.
Os dados das janelas estão em [performance-linux-0.1.1.json](performance-linux-0.1.1.json).
As imagens, o disco e o código derivado da ROM ficam apenas no computador local.

## Verificação da atualização

Código: `d9a35bda8de34a44fd608b2e99bb829e8307ed09`.
24 testes locais passaram, incluindo GPU com driver ausente, timeout, opções PRIME
externas, preferências inválidas e preservação do cache/configurações.
A [validação automática Linux](https://github.com/GenissonEmilio/GodOfWar2PC/actions/runs/39861007094) também passou.
A interface congelada 0.1.1 foi renderizada e inspecionada na página Gráficos.
O aplicativo instalado passou por mais 150 segundos de abertura via interface,
com sete capturas, RTX selecionada automaticamente, textura compartilhada e saída normal,
sem `missing-target`. Esse teste adicional cobre abertura/renderer; o combate
foi medido separadamente nas três execuções acima.
A checagem do cache depois da mudança de pasta confirmou que o executável,
o marcador de compilação e o log de preparação não mudaram.

O escopo continua sendo o início do jogo. A campanha completa, os chefes posteriores
e todas as configurações de hardware não foram validados. Consulte também a
[validação da base 0.1.0](LINUX-VALIDATION.md).
