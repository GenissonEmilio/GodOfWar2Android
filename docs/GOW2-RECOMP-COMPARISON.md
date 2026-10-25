# Comparação com gow2-recomp

Análise de 25/10/2026. Referência: [andrebrumdev/gow2-recomp,
commit a7c1f64](https://github.com/andrebrumdev/gow2-recomp/tree/a7c1f64c825a04bce836059671d8deb2ec0db983).

## O que a referência comprova

O projeto parte de **GoW II HD de PS3, NPUA80491**, traduz PPU/SPU e usa Vulkan
no Android. Nosso projeto parte de **PS2, SCUS-97481**, com EE/IOP/VU e GLES.
Os executáveis, estados gráficos e dados dos dois consoles não são intercambiáveis.

O [README da referência](https://github.com/andrebrumdev/gow2-recomp/blob/a7c1f64c825a04bce836059671d8deb2ec0db983/README.md#estado-por-plataforma-e-desempenho-2026-10-02)
registra Android experimental: 12/11 FPS (p50/p5) no Galaxy Tab S9 FE,
ou 17/15 com thread RSX e fragmentos experimentais de 32 KiB. A cinemática
atinge 29–30 FPS; isso não demonstra combate a 30 FPS.

O runtime `andrebrumdev/ps3recomp`, referenciado pelo build, retornou HTTP 404
nesta análise. Portanto, as implementações internas e os relatórios ligados a
esse endereço não foram inspecionados. Os resultados acima são relatos do autor,
sem reprodução local. Nenhum script da referência foi executado.

## Aplicabilidade ao nosso runtime

| Técnica relatada na referência | Situação no nosso port | Decisão |
| --- | --- | --- |
| Fragmentação e execução gráfica em paralelo | Estágio GIF e fila GS já publicam blocos de 64 KiB, com limite e espera de consumo | Preservar barreiras e ordenação; o ganho do RSX não pode ser transferido numericamente para o GS |
| Evitar releitura de memória Vulkan mapeada | Nossos vértices ficam em vetores de CPU enviados por `BufferData`; não há leitura de índices em memória Vulkan | Não há o mesmo acesso para substituir |
| Deduplicar comandos e estado | Fila compacta, cache de uniforms e de estado GLES já existem | Ampliar o reaproveitamento do snapshot entre frontend, fila e preparação dos vértices |
| Cache persistente de pipelines Vulkan | Não existe pipeline Vulkan neste backend | Exige implementação GLES própria ou novo backend; não é copiar uma opção |
| Otimização do interpretador SPU | Nosso trabalho vetorial usa VU1 e fontes nativos locais | Medir VIF/VU; código SPU não executa programas VU |
| Afinidade, trabalhadores e flags de compilação | Temos quatro trabalhadores registrados para ADPF e compilação `-O3` | Não fixar núcleos nem reduzir precisão sem evidência de ganho |

Fontes do port de referência:
[build Android](https://github.com/andrebrumdev/gow2-recomp/blob/a7c1f64c825a04bce836059671d8deb2ec0db983/android/build_android.sh),
[flags ARM64](https://github.com/andrebrumdev/gow2-recomp/blob/a7c1f64c825a04bce836059671d8deb2ec0db983/build_macos.sh),
[registro SPU](https://github.com/andrebrumdev/gow2-recomp/blob/a7c1f64c825a04bce836059671d8deb2ec0db983/recomp_mid_v2/gow2_spu_register.c),
[medição Android](https://github.com/andrebrumdev/gow2-recomp/blob/a7c1f64c825a04bce836059671d8deb2ec0db983/android/run_perf.sh).

## Mudança desenvolvida aqui

O perfil de combate do POCO coletou 19.877 amostras sem perda. A comparação do
estado aparece em 2,67% das amostras globais no GS e 1,72% no frontend, com
contadores gráficos ativos. O diagnóstico anterior também repetia comparações.
Essas porcentagens não equivalem a ganho possível de FPS.

O frontend já reconstrói o snapshot somente quando há escrita de estado.
Agora ele identifica cada snapshot imutável com um token. Coordenadas, cores e
outros atributos dos vértices continuam sendo enviados em todos os desenhos.
A fila reutiliza a identidade e gera outra identidade para seu replay. O
renderizador evita comparar todos os campos quando o token coincide; chamadas
sem token mantêm a comparação completa. Tokens são únicos entre instâncias e
resets. Não há comparação por hash sujeita a colisões.

Isso é uma implementação própria, motivada pela comparação das arquiteturas e
pelo perfil local. Não foi transplantado código PS3 nem alterado o conteúdo do
disco. A referência está sob MIT; nosso runtime continua sob suas licenças GPL.

Os resultados dos testes de pixels, ordenação e combate ficam em
[ANDROID-VALIDATION.md](ANDROID-VALIDATION.md). A meta só será cumprida quando
o jogo produzir quadros reais em até 33,3 ms de forma sustentada no aparelho.
