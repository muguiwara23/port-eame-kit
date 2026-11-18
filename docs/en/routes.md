# How a game gets to the iPhone

[Español](../es/rutas.md)

## The translator already exists: the compiler

An iPhone runs arm64 code. Apple's compiler (clang) translates C and C++ straight to arm64 for iOS, just as it does for Mac or Windows. There is no need to move the game to Swift: Swift is another language that also ends up as arm64, and many iOS apps and games have a C or C++ core. Rust compiles for iOS too (target `aarch64-apple-ios`).

That is why Port/eame doesn't translate languages. What changes from PC to iPhone is the platform: screen, controls, files, life cycle and graphics. That is the kit.

## What can't be done: translating while you play

iOS doesn't let an app generate new code while it runs (JIT) without debugging tools. A fast emulator translates the console's code while you play. That is why here everything must be translated before it is compiled.

## Depending on where the game comes from

| Source | Example | Route |
| --- | --- | --- |
| Decomp: the game's code rebuilt in C or C++ | EarthBound (Phase Distorter) | Compile with clang + kit |
| Static recomp: the console's program turned into C or C++ | Yu-Gi-Oh! Forbidden Memories (PS1, MIPS → C) | Same: the result is C or C++ |
| Recomps of other consoles (N64, Xbox 360, Switch…) | Projects such as N64Recomp or XenonRecomp | Same; check which graphics it uses |
| Port written in Rust | — | Compile for `aarch64-apple-ios` + kit |
| Java phone games (J2ME) | — | A JIT-less interpreter that runs the `.jar` |
| Emulator with live translation (dynarec) | — | Not a Port/eame route: it needs JIT |

## Switch and ARM

The Switch uses the same architecture as the iPhone (ARM64), but its program doesn't run as is: it asks things of the Switch's system and of its GPU (NVN), which the iPhone doesn't have. A recomp turns that program into C or C++ and adds a layer that answers in the Switch's place. Then clang compiles it to arm64 again, for iOS. In the end it is still ARM, and the code stays portable.

## Graphics: translators that already exist

| What the game uses | On iPhone |
| --- | --- |
| OpenGL ES 2/3 | Directly (the kit's base) |
| SDL_Renderer or SDL_GPU | SDL uses Metal by itself |
| Desktop OpenGL (the old 1.x–2.x) | gl4es turns it into OpenGL ES |
| Vulkan | MoltenVK turns it into Metal |
| If Apple removes OpenGL ES | ANGLE turns OpenGL ES into Metal, without touching the game |
| Direct3D | Case by case; many recomps already ship Vulkan |

None of them needs JIT: they are libraries compiled into the app.
