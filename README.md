# God of War II para Android

Adaptação **experimental, em desenvolvimento** da versão PS2 USA SCUS-97481
v1.01 para Android ARM64. Derivada de
[GodOfWar2PC](https://github.com/GenissonEmilio/GodOfWar2PC) e
[cxldsakeb/GoW2-nativo-PS2](https://github.com/cxldsakeb/GoW2-nativo-PS2).
A experiência móvel do [Black Android](https://github.com/RanggaCasper/Black-Android)
é referência de interface e controles; o Black usa Xbox e seu motor não executa GoW II.

**Este projeto ainda não é um port concluído.** Compilar, iniciar e jogar o primeiro
cenário não comprovam a compatibilidade da campanha inteira. Resultados reais e
pendências ficam em [docs/ANDROID-VALIDATION.md](docs/ANDROID-VALIDATION.md).

O executável PS2 é traduzido antecipadamente para C++ pelo PS2Recomp, compilado
pelo NDK para ARM64. O runtime atende as APIs PS2, GS, IOP e VU; não depende de BIOS.
Os fontes gerados do jogo e seus arquivos de disco ficam locais.

## Experiência móvel em implementação

- Interface em português com tema escuro, vermelho e dourado.
- Importação local de ISO/ZIP DVD9 com verificação do executável e das duas camadas.
- Dois analógicos touch, botões PS2, D-pad e controles simultâneos por multitoque.
- Editor de posições por arraste, tamanho por botão e persistência do layout.
- Menu de pausa sobre o jogo com opacidade, tamanho, zona morta, sensibilidade,
  volume e limite da apresentação em 30/60 FPS.
- Resolução interna 1×, 2× ou 3× aplicada ao reiniciar o jogo.
- Entrada de controles Android por eixos, gatilhos e botões; ocultação automática
  do touch. Contador baseado nos frames do jogo.
- Saves em cartão virtual dentro da pasta do app e pausa ao ir para segundo plano.
- Renderização OpenGL ES 3.2 com contextos EGL compartilhados. A leitura do
  framebuffer usa cópias explícitas, sem exigir barreiras de textura de desktop.

A lista descreve o código implementado; a validação funcional é registrada
separadamente. Os ajustes móveis são uma interface sobreposta ao jogo. Ainda não
há integração dessas páginas às opções originais renderizadas pelo executável PS2.

## Compilar localmente

Requer Linux, Python 3, JDK 17/21, SDK 35, NDK `28.2.13676358` e CMake `3.31.6`.
Não há download de dados do jogo. Não são publicados APKs com código derivado do DVD.

1. Construa o `ps2_recomp` da base PS2Recomp para seu computador ou use seu kit
   local GodOfWar2PC. O processo está no [README da base PC](docs/PC-README.md).
2. Gere os fontes a partir de seu disco:

   ```sh
   python3 tools/android/prepare.py --disc '/caminho/God of War II (USA).zip' \
     --recompiler /caminho/ps2_recomp
   ```

   Opcional: `--native-cache /caminho/native` reutiliza os fontes VU1/IOP gerados
   pelo seu próprio cache GodOfWar2PC. Sem esse cache, as rotinas correspondentes
   usam os caminhos de interpretação existentes na base, com custo de desempenho.
3. Defina `ANDROID_HOME` e execute `tools/android/build.sh`.
4. Instale `android/app/build/outputs/apk/debug/app-debug.apk` no aparelho.
5. Abra God of War II e importe o mesmo ISO/ZIP no app. A importação precisa
   de aproximadamente 18 GiB livres temporários; depois permanecem ~8 GiB.

Os arquivos ficam em `Android/data/io.github.genissonemilio.gow2/files`:
`game/`, `memcards/` e configurações. O layout e os ajustes usam as preferências
privadas do Android. Não limpe os dados do aplicativo para preservar os saves.

## Testes sem jogo

```sh
javac -d /tmp/gow2-check android/app/src/main/java/io/github/genissonemilio/gow2/DiscImporter.java tests/android/DiscImporterTest.java
java -cp /tmp/gow2-check io.github.genissonemilio.gow2.DiscImporterTest
python3 -m unittest discover -s tests/linux -v
```

## Créditos

GPL-3.0, incluindo as licenças da base e suas dependências. PS2Recomp: ran-j e
colaboradores; GoW II: cxldsakeb, DaniSBR1 e Alejrr2; runtime: Waffle565;
renderizador: Taylor N. Albarnaz/LightVelox; referência de hardware: PCSX2;
raylib e sse2neon: seus respectivos autores. Histórico em [docs/NOTES.md](docs/NOTES.md).
God of War II pertence à Sony/Santa Monica Studio. Projeto independente.
