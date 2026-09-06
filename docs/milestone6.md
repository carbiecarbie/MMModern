# Milestone 6: Clouds Area A1

O modo abaixo carrega o estado original do mapa 001 de `xeen.cc` e termina
depois de imprimir o relatorio. Nao inicializa SDL, cria framebuffer ou abre janela.

```bash
cd /d/Projetos/MModern/mmodern
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/mmodern.exe --inspect-map "F:/Games/gog/Might and Magic 4-5"
```

O CMake regenera as regras automaticamente. Os testes de formato sao controlados
por `BUILD_TESTING` (ON por padrao) e usam somente C++ padrao.

## Fluxo implementado

- `src/compat/scummvm/ScummVmXeenBridge.cpp`: abre `xeen.cc` com CCArchive;
  concatena os blocos presentes `2a0c`, `2a1c`, `2a2c`, `2a3c`, `284c`, `2a5c`
  nessa ordem. InitialCloudsArchive usa BaseCCArchive para ler o indice interno,
  verifica limites e retorna os membros em memoria, sem aplicar XOR novamente.
- `src/formats/xeen/XeenAssetSource.{h,cpp}`: fornece bytes por uma API padrao;
  seu construtor sem dimensoes permite acesso sem framebuffer.
- `src/formats/xeen/XeenMapFormat.{h,cpp}`: parsers puros de DAT, MOB e EVT.
- `src/games/xeen/XeenMap.h`: dados de geometria, tabelas, entidades e instrucoes,
  sem tipos ScummVM/SDL. Palavras e atributos originais sao preservados.
- `src/games/xeen/XeenMapLoader.{h,cpp}`: carrega exclusivamente `maze0001.dat`,
  `maze0001.mob` e `maze0001.evt` e valida o ID interno 001.
- `src/app/Application.cpp`: imprime resumo e grade, com Y=15 no topo.

Nenhum XEEN.CUR, save, mm.dat, HED, TXT ou mapa vizinho e carregado. O modo
grafico anterior permanece disponivel usando apenas o diretorio como argumento.

## Valores verificados na instalacao real

| Dado | Resultado |
|---|---|
| ID / tipo | 001 / exterior |
| Dimensoes / celulas | 16x16 / 256 |
| Objetos de cenario | 7 registros, 7 ativos |
| Monstros | 11 registros, 11 ativos |
| Objetos de parede | 1 registro, 0 ativos, 1 sem recurso |
| Instrucoes EVT | 60 |
| Vizinhos N/E/S/W | nenhum / 005 / 002 / nenhum |

Correcao da analise inicial: o registro de parede e `(99,99,0,0)`, mas a tabela
correspondente contem somente FF. O leitor original ignora essa referencia sem
sprite. MMModern preserva o registro bruto e informa que ele nao e ativo.
Nenhuma dessas contagens esta codificada no programa.

Entidades com X ou Y igual a -128 permanecem na ordem original e sao contadas
como desativadas. "Ativo" significa referencia resolvida e ausencia desse
marcador; nao significa que esteja visivel ou dentro da grade 16x16. Por exemplo,
um objeto original da Area A1 tem Y=-2, que deve ser preservado.

A grade usa indices locais em hexadecimal. A tabela local -> tipo de superficie
e impressa abaixo dela. As 60 entradas EVT sao instrucoes; seus opcodes e
parametros permanecem opacos, sem agrupamento ou execucao de scripts.

## Fidelidade e referencias

Os caminhos ScummVM abaixo sao relativos a `D:/Projetos/MModern/scummvm-master`:

- `engines/mm/xeen/files.cpp`: SaveArchive::reset, ordem dos blocos iniciais;
  SaveArchive::createReadStreamForMember, payload interno sem XOR.
- `engines/mm/shared/xeen/cc_archive.cpp`: CCArchive e BaseCCArchive originais.
- `engines/mm/xeen/map.cpp`: MazeData::synchronize (DAT de 892 bytes),
  MobStruct e MonsterObjectData::synchronize (MOB).
- `engines/mm/xeen/scripts.cpp`: MazeEvent::synchronize (EVT).
- `devtools/create_mm/create_xeen/constants.cpp`: WALL_SHIFTS.

Interiores usam paredes N/E/S/W nos bits 12/8/4/0, conforme WALL_SHIFTS e
Map::setWall. Nao usamos a ordem dos bitfields C++ declarados no map.h como
especificacao binaria. Exteriores usam superficie/intermediario/topo/overlay
nos bits 0/4/8/12; o byte paralelo fornece flags e a superficie interior.

Os parsers rejeitam tamanhos, comprimentos, indices fora dos 16 slots e
terminadores incompletos. Preservam holes na tabela de objetos e usam a tabela
compactada de monstros/objetos de parede, como a engine. Referencias nao
resolvidas sao mantidas explicitamente como resourceId=-1.

## Regressao grafica automatizada

O alvo de teste abaixo chama o mesmo Application::run do modo grafico, com os
recursos reais. O driver dummy exercita composicao, textura/apresentacao SDL e
fechamento normal sem abrir janela. Nao substitui uma inspecao visual manual.

```bash
cmake --build build --target mmodern_graphics_smoke --parallel 4
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/mmodern_graphics_smoke.exe "F:/Games/gog/Might and Magic 4-5" escape
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/mmodern_graphics_smoke.exe "F:/Games/gog/Might and Magic 4-5" quit
```

Para testar visualmente o executavel:

```bash
./build/mmodern.exe "F:/Games/gog/Might and Magic 4-5"
```

A inspecao tambem foi executada com SDL_VIDEODRIVER deliberadamente invalido;
terminou com codigo 0. Os testes sinteticos cobrem os marcadores -128, listas
vazias duplas, placeholders, nibbles, endian, bitmaps e arquivos truncados.
