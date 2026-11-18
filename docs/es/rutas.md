# Cómo llega un juego al iPhone

[English](../en/routes.md)

## El traductor ya existe: el compilador

Un iPhone ejecuta código arm64. El compilador de Apple (clang) traduce C y C++ directo a arm64 para iOS, igual que lo hace para Mac o Windows. No hace falta pasar el juego a Swift: Swift es otro lenguaje que también termina en arm64, y muchas apps y juegos de iOS tienen su núcleo en C o C++. Rust también compila para iOS (destino `aarch64-apple-ios`).

Por eso Port/eame no traduce lenguajes. Lo que cambia de PC a iPhone es la plataforma: pantalla, controles, archivos, ciclo de vida y gráficos. Eso es el kit.

## Lo que no se puede: traducir mientras se juega

iOS no deja que una app genere código nuevo mientras corre (JIT) sin herramientas de depuración. Un emulador rápido traduce el código de la consola mientras juegas. Por eso aquí todo tiene que estar traducido antes de compilar.

## Según de dónde viene el juego

| Origen | Ejemplo | Ruta |
| --- | --- | --- |
| Decomp: el código del juego reconstruido en C o C++ | EarthBound (Phase Distorter) | Compilar con clang + kit |
| Recomp estática: el programa de la consola convertido en C o C++ | Yu-Gi-Oh! Forbidden Memories (PS1, MIPS → C) | Igual: el resultado es C o C++ |
| Recomps de otras consolas (N64, Xbox 360, Switch…) | Proyectos como N64Recomp o XenonRecomp | Igual; revisar qué gráficos usa |
| Port escrito en Rust | — | Compilar para `aarch64-apple-ios` + kit |
| Juegos Java de celular (J2ME) | — | Un intérprete sin JIT que corre el `.jar` |
| Emulador con traducción en vivo (dynarec) | — | No es ruta de Port/eame: necesita JIT |

## Switch y ARM

La Switch usa la misma arquitectura que el iPhone (ARM64), pero su programa no corre tal cual: le pide cosas al sistema de la Switch y a su GPU (NVN), que el iPhone no tiene. Una recomp convierte ese programa en C o C++ y le agrega una capa que responde en lugar de la Switch. Después clang lo compila otra vez a arm64 para iOS. Al final sigue siendo ARM, y el código queda portátil.

## Gráficos: traductores que ya existen

| Lo que usa el juego | En iPhone |
| --- | --- |
| OpenGL ES 2/3 | Directo (la base del kit) |
| SDL_Renderer o SDL_GPU | SDL usa Metal por su cuenta |
| OpenGL de PC (el viejo, 1.x–2.x) | gl4es lo convierte en OpenGL ES |
| Vulkan | MoltenVK lo convierte en Metal |
| Si Apple retira OpenGL ES | ANGLE convierte OpenGL ES en Metal, sin tocar el juego |
| Direct3D | Caso por caso; muchas recomps ya traen Vulkan |

Ninguno necesita JIT: son bibliotecas que se compilan dentro de la app.
