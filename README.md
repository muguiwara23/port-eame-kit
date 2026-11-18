# Port/eame kit

**Español** · [English](#english)

El kit de **Port/eame**: todo lo que un juego necesita para volverse una app nativa de iPhone y iPad, sin JIT, escrito una sola vez y reutilizado por cada port.

## Propósito

Traer un juego nuevo a iOS no debe significar escribir código cada vez. Con el kit, un port nuevo es:

1. Elegir un proyecto que ya tenga el juego traducido a C, C++ o Rust (una decomp o una recomp).
2. Llenar la ficha del juego (`porteame.json`) y conectar un adaptador pequeño.
3. Correr el Action de GitHub, instalar el IPA y revisarlo contra el contrato.

Cada problema que aparece en un port se resuelve aquí una vez y queda escrito, para que el siguiente juego lo tenga resuelto desde el inicio.

## Documentos

- [Contrato](docs/es/contrato.md): lo que toda app de Port/eame hace igual.
- [Rutas](docs/es/rutas.md): cómo llega un juego al iPhone según de dónde viene.
- [Lecciones](docs/es/lecciones.md): cada problema que ya resolvimos, con su causa y su solución.
- [Cómo usar el kit](docs/es/uso.md): el submódulo, la ficha, el script, el Action y el código.

## Qué trae

| Parte | Dónde |
| --- | --- |
| Empaquetador del IPA, leído de la ficha `porteame.json` | `tools/package_ipa.py` |
| SDL3 para iOS, opciones de CMake y revisión del ejecutable | `tools/ios_env.sh` |
| El Action de GitHub que usa cada fork | `actions/ios/action.yml`, `templates/port-eame-ios.yml` |
| Ícono personal cifrado | `tools/port_eame_private.py` |
| Utilidades de iOS en C (registro, Documents, segundo plano, cerrar) | `include/porteame/ios.h` |
| Controles táctiles en C, SNES y PS1, con dibujo en OpenGL ES y SDL_Renderer | `include/porteame/touch*.h` |
| Plantillas para un juego nuevo | `templates/` |
| Pruebas en Linux | `tests/` |

## Estado

**v0.1.0.** Probado en Linux: la lógica de los controles, las utilidades de iOS, el empaquetador (da el mismo IPA que los de Yu-Gi-Oh! y EarthBound) y el dibujo de los controles. Falta la prueba en iPhone: primero EarthBound pasa al kit, después Yu-Gi-Oh!.

## Reglas

- Nunca se incluyen ROMs, imágenes de disco, paquetes de datos ni partidas, ni en un repositorio ni en un IPA. Cada jugador usa su propia copia.
- Sin JIT: todo se traduce antes de compilar.
- Los íconos personales viven solo cifrados; las versiones públicas van sin ellos.
- En los forks se trabaja solo en ramas `port-eame/…`; la rama principal y las etiquetas del proyecto original no se tocan.

## Licencia

MIT (ver [LICENSE](LICENSE)). Cada juego y cada componente conserva su propia licencia.

---

## English

The **Port/eame** kit: everything a game needs to become a native iPhone and iPad app, with no JIT, written once and reused by every port.

### Purpose

Bringing a new game to iOS should not mean writing code every time. With the kit, a new port is:

1. Pick a project that already has the game translated to C, C++ or Rust (a decomp or a recomp).
2. Fill in the game's sheet (`porteame.json`) and hook up a small adapter.
3. Run the GitHub Action, install the IPA and check it against the contract.

Every problem that shows up in a port is solved here once and written down, so the next game has it solved from the start.

### Documents

- [Contract](docs/en/contract.md): what every Port/eame app does the same way.
- [Routes](docs/en/routes.md): how a game gets to the iPhone, depending on where it comes from.
- [Lessons](docs/en/lessons.md): every problem already solved, with its cause and its fix.
- [How to use the kit](docs/en/usage.md): the submodule, the sheet, the script, the Action and the code.

### What it has

| Part | Where |
| --- | --- |
| The IPA packager, driven by the `porteame.json` sheet | `tools/package_ipa.py` |
| SDL3 for iOS, CMake options and the executable's check | `tools/ios_env.sh` |
| The GitHub Action each fork uses | `actions/ios/action.yml`, `templates/port-eame-ios.yml` |
| Encrypted personal icon | `tools/port_eame_private.py` |
| iOS helpers in C (log, Documents, background, closing) | `include/porteame/ios.h` |
| Touch controls in C, SNES and PS1, drawn with OpenGL ES or SDL_Renderer | `include/porteame/touch*.h` |
| Templates for a new game | `templates/` |
| Tests on Linux | `tests/` |

### Status

**v0.1.0.** Tested on Linux: the controls' logic, the iOS helpers, the packager (it makes the same IPA as the Yu-Gi-Oh! and EarthBound ones) and the controls' drawing. Not yet tested on an iPhone: EarthBound moves to the kit first, then Yu-Gi-Oh!.

### Rules

- No ROMs, disc images, data packs or saves are ever included, in a repository or in an IPA. Each player uses their own copy.
- No JIT: everything is translated before it is compiled.
- Personal icons live only encrypted; public builds ship without them.
- In the forks, work happens only on `port-eame/…` branches; the original project's main branch and tags are never touched.

### License

MIT (see [LICENSE](LICENSE)). Each game and each component keeps its own license.
