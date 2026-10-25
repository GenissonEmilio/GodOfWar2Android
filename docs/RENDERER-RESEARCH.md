# Pesquisa de desempenho: PS2 no Android

Investigação de 25/10/2026, com commits fixados. Os fontes externos foram lidos,
sem executar scripts. As mudanças neste port são implementações próprias.

## Conclusão aplicada ao POCO

As otimizações anteriores não comprovaram ganho significativo no combate. O
`opt29` registrou 16,484; 16,378; 16,491; 15,576; 14,750; 15,056 FPS nas seis
janelas após a transição. A meta de 30 FPS sustentados continua pendente.

A consulta EGL no aparelho confirmou `GL_EXT_blend_func_extended`,
`GL_EXT_buffer_storage` e `GL_EXT_shader_framebuffer_fetch`. Nosso adaptador GLES
removia o segundo resultado do fragment shader e forçava mistura por shader
sempre que a equação precisava desse resultado. Essa limitação de plataforma
também era aplicada ao Adreno 810, que anuncia o recurso necessário.

## Referências e decisões

| Projeto e fonte | Técnica verificada no código | Aplicação aqui |
| --- | --- | --- |
| [PCSX2, `EmulateBlending`](https://github.com/PCSX2/pcsx2/blob/c6cc5160f914f8788fa7fe30c5d52bcd4d2e5c20/pcsx2/GS/Renderers/HW/GSRendererHW.cpp#L6890) | Escolhe mistura por hardware/shader usando a equação e limites do alpha; simplifica fatores constantes | Prioridade: detectar a extensão real e provar limites do alpha antes de usar mistura com duas saídas |
| [Black Android, buffers por frame](https://github.com/RanggaCasper/Black-Android/blob/c1b7fb0079cb1c0bad1c375ef4b983f00a436399/native/vulkan.cpp#L1010) | Memória de upload mapeada e reutilizada; semáforos/fences protegem a reutilização | Próxima experiência: anel de vértices com mapeamento persistente no GLES, comparado ao `BufferData` por batch |
| [Black Android, cache](https://github.com/RanggaCasper/Black-Android/blob/c1b7fb0079cb1c0bad1c375ef4b983f00a436399/native/vulkan.cpp#L1566) | Carrega cache de pipelines entre execuções | Útil contra engasgos de compilação; não demonstra aumento do FPS de uma cena já aquecida |
| [Play!, renderizador móvel](https://github.com/jpd002/Play-/blob/505dac73149fece41b04dd961078ce3011a8bed5/Source/gs/GSH_Vulkan/GSH_VulkanDrawMobile.cpp#L125) | Agrupa vértices, usa pipelines por estado, input attachments e subpasses; preserva dependências entre desenhos | Referência para especializar shaders e manter operações locais ao render target. Um backend Vulkan completo exige portar armazenamento, sincronização e apresentação |

O Black parte do Xbox e traduz chamadas D3D8 para Vulkan. Seu renderer não
interpreta os estados do GS do PS2. O Play! mantém outra representação da memória
GS. Nenhum deles fornece uma troca de backend pronta para este port. A escolha de
Vulkan, isoladamente, não estabelece um ganho numérico no POCO.

## Primeira mudança: duas saídas do fragment shader

O `opt31` habilita o recurso somente se o driver o anunciar e a variante do shader
for compilada/ligada com sucesso. As passagens que leem o destino por DATE ou Z,
usam mistura não representável ou podem produzir alpha maior que 128 conservam o
caminho por shader. A faixa do alpha considera os vértices, TFX/TCC, imagem RGBA
decodificada ou CLUT; render targets desconhecidos usam um limite conservador.
Uma transferência que substitui a imagem ou uma mudança de CLUT também substitui
os limites usados. `perf-dual-off` fornece a comparação na mesma compilação.

Valores de FIX acima de 128 também usam a equação no shader: limitar o fator GL
a 1 mudaria o resultado. A [especificação Khronos](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_blend_func_extended.txt)
define as duas saídas e os fatores SRC1; não torna todas as equações do GS
representáveis por hardware.

O teste compara RGBA com um cálculo independente da equação do GS: 640 casos de
TFX/TCC, alpha de vértice/texel, imagem direta/paleta e fator As/FIX, incluindo
129 e 255. Ele é repetido com o recurso ligado/desligado e com framebuffer fetch
ou cópia. Além desse teste, é necessária a medição no combate e a inspeção visual.
Os resultados efetivos ficam em [ANDROID-VALIDATION.md](ANDROID-VALIDATION.md).

## Segunda mudança e limite observado

O `opt32` implementou o anel persistente, com quatro regiões e fences antes da
reutilização. A mediana observada passou de 17,730 a 18,760 FPS na mesma sequência
de entrada, em execuções separadas. O perfil posterior coloca VUWorker à frente
do GS nos ciclos de CPU. Isso limita o benefício esperado de continuar alterando
somente o renderer; a próxima investigação deve medir VIF e a execução dos
blocos vetoriais nativos. Dados e ressalvas estão na validação.
