# God of War II PC — Linux

Projeto de recompilação estática da versão de PS2 **USA SCUS-97481 v1.01**, com
interface em português, importação local do DVD e compilador Linux incluído.
Derivado de [cxldsakeb/GoW2-nativo-PS2](https://github.com/cxldsakeb/GoW2-nativo-PS2),
com a experiência de instalação do [ShadowOfTheColossusPC](https://github.com/GenissonEmilio/ShadowOfTheColossusPC).

**Em desenvolvimento:** a base upstream foi testada no início de Rhodes e tem
limitações gráficas e de desempenho. Não há validação da campanha inteira. Os
resultados desta adaptação Linux estão em [docs/LINUX-VALIDATION.md](docs/LINUX-VALIDATION.md).

## Usar

1. Baixe o pacote Linux em [Releases](https://github.com/GenissonEmilio/GodOfWar2PC/releases).
2. Extraia e abra `GodOfWar2PC`.
3. Escolha sua cópia em ISO, ZIP, RAR ou 7Z. O importador verifica o executável e
   as duas camadas do DVD. Outras regiões ou executáveis modificados são rejeitados.
4. Aguarde a compilação e a otimização automática inicial. São usadas até duas
   tarefas de compilação. Pode levar bastante tempo na primeira preparação.
5. Clique em **Iniciar jogo**. As próximas aberturas reutilizam o cache validado.

O pacote inclui GTK, GCC 13, PS2Recomp, fontes e bibliotecas necessárias. Não exige
Python, Docker, BIOS de PS2 ou SDK instalado para usar o pacote. Requisitos:
Linux x86_64 com SSE4.1, glibc 2.39 ou superior, OpenGL 4.6, 8 GB de RAM e 25 GB livres
para importar um arquivo compactado e compilar. Ubuntu 24.04+ e Fedora 40+ são as bases.

**Dados e saves:** `~/.local/share/GodOfWar2PC` (respeita `XDG_DATA_HOME`). Após a
preparação, o ZIP/ISO original é dispensável; preserve os dados importados e saves.
A ROM não é alterada nem enviada para a internet. Nenhum arquivo do jogo ou código
recompilado é distribuído no repositório ou nos releases.

## Imagem e controles

A aba **Gráficos** oferece janela/tela cheia na resolução do monitor, proporção
16:9 ou esticar, e fragment shaders GLSL 330 importados e validados no driver local.
Há um filtro de nitidez pronto. Shaders são pós-processamento, não texturas HD.
O jogo usa a câmera panorâmica Hor+ da base upstream.

| Ação | Tecla |
|---|---|
| Mover | WASD |
| Esquiva / analógico direito | IJKL |
| □ / ✕ / ○ / △ | Z / X / C / V |
| L1 / R1 | Q / E |
| L2 / R2 | 1 / 3 ou Shift esquerdo / direito |
| Start / Select | Enter / Tab |
| Tela cheia | F11 ou F12 |
| FPS | F10 |

Controles USB/Bluetooth usam o suporte nativo da raylib/GLFW. Conecte antes de jogar.
Os botões do controle mantêm o esquema original de PS2.

## Desenvolver

`tools/linux/build.sh` usa Docker para construir o runtime, o compilador portátil,
o launcher congelado e o pacote sem ROM. A integração contínua testa importação,
cache, compilação de dados sintéticos e apresentação OpenGL em monitores virtuais.
Testes: `python3 -m unittest discover -s tests/linux -v`.
A validação com uma cópia local usa `tools/linux/validate_local.py`; arquivos
recompilados ficam apenas na pasta de dados do usuário.

## Créditos e licença

GPL-3.0. Base GoW II: cxldsakeb, DaniSBR1 e Alejrr2; PS2Recomp: ran-j e colaboradores;
runtime: Waffle565; contexto OpenGL Linux: YYOzcan/Tobiichi-Port; referência de
hardware: PCSX2. A interface e o kit portátil derivam dos projetos de GenissonEmilio
ShadowOfTheColossusPC, ForbiddenMemoriesPC e CrashBandicootPC. Procedência em
[docs/NOTES.md](docs/NOTES.md). O código correspondente e as licenças acompanham o pacote.

God of War II pertence à Sony / Santa Monica Studio. Projeto independente.
