# Cómo usar el kit

[English](../en/usage.md)

Lo que se hace una sola vez en el fork de cada juego. Después, cada IPA nuevo es solo correr el Action.

## 1. Traer el kit como submódulo

En la rama `port-eame/ios` del fork:

```bash
git submodule add https://github.com/muguiwara23/port-eame-kit port-eame-kit
git -C port-eame-kit checkout v0.1.0
git add .gitmodules port-eame-kit
```

El kit queda fijo en esa versión. Para subirlo más adelante: `git -C port-eame-kit checkout v0.2.0` y un commit. Así una versión nueva del kit nunca rompe un juego que ya funciona.

## 2. La ficha del juego: `porteame.json`

En la raíz del fork. Hay una plantilla en `templates/porteame.json`.

| Campo | Qué es |
| --- | --- |
| `porteame` | El formato de la ficha: `1` |
| `app.name` | Nombre del `.app` y `CFBundleName` |
| `app.display_name` | El nombre bajo el ícono (si falta, `name`) |
| `app.executable` | El nombre del ejecutable dentro del `.app` |
| `app.bundle_id` | `org.porteame.<juego>` |
| `app.version` | La versión del juego, por ejemplo `0.5` |
| `app.minimum_ios` | El iOS más viejo (por defecto `16.0`) |
| `ipa` | El nombre del IPA; `{version}` y `{executable}` se reemplazan |
| `licenses` | `"nombre en la app": "archivo"`, van a la carpeta `licenses` |
| `files` | `"dónde en la app": "archivo o carpeta"`, otros archivos que el juego necesita |
| `forbidden` | Extensiones que nunca deben entrar, además de las del kit |
| `icon` | El ícono cifrado, por ejemplo `ios/private/app-icon.png.enc` |
| `plist` | Llaves del Info.plist que se agregan o cambian |

Las rutas son relativas a la ficha y pueden usar variables, como `${PORTEAME_SDL3_SOURCE}/LICENSE.txt` (la licencia de SDL3).

El kit siempre rechaza ROMs, imágenes de disco y partidas (`.sfc .smc .bin .cue .iso .chd .z64 .n64 .v64 .nsp .xci .srm .sav .state`), aunque la ficha no lo diga.

## 3. El script de compilación

`ios/build_ios.sh` (plantilla en `templates/build_ios.sh`): carga `tools/ios_env.sh` del kit, compila SDL3, compila el juego con CMake para iOS, revisa que el ejecutable sea de iOS arm64 y arma el IPA. El juego solo cambia sus opciones de CMake y el nombre de su ejecutable.

## 4. El Action

Copiar `templates/port-eame-ios.yml` a `.github/workflows/port-eame-ios.yml`. Usa la acción del kit (`port-eame-kit/actions/ios`), con botón manual, opción de Mac Intel o Apple Silicon y la casilla del ícono. El ícono se descifra con el secreto `PORTEAME_ICON_KEY` del repositorio.

Recuerda: el botón "Run workflow" solo aparece si el workflow está en la rama por defecto del fork (`port-eame/ios`). Para probar otra rama, elígela en "Use workflow from".

## 5. El código del juego

En el `CMakeLists.txt` del juego:

```cmake
add_subdirectory(port-eame-kit)
target_link_libraries(mi_juego PRIVATE porteame::kit)
```

### Utilidades de iOS: `porteame/ios.h`

| Función | Para qué |
| --- | --- |
| `pe_ios_startup(nombre, carpetas)` | Lo primero en `main`: Documents, registro `port-eame-log.txt`, `environment.txt`, horizontal |
| `pe_ios_find_file(carpeta, extensiones, …)` | Encontrar el juego del jugador en Documents |
| `pe_ios_window_flags()` | Pantalla completa sin bordes en iOS |
| `pe_ios_density(ventana)` / `pe_ios_safe_area(…)` | Píxeles por punto y los bordes seguros, en píxeles |
| `pe_ios_is_background(evento)` + `pe_ios_wait_for_foreground(guardar, datos)` | Guardar y pausar en segundo plano |
| `pe_ios_error(título, mensaje)` | Un error que el jugador ve |
| `return pe_ios_finish(estado);` | El final de `main`: en iOS termina la app de verdad |

Fuera de iOS no hacen nada, o hacen lo normal, así que no hacen falta `#ifdef`.

### Controles táctiles: `porteame/touch.h`

```c
PeTouch *touch = pe_touch_create(PE_TOUCH_SNES);        /* o PE_TOUCH_PS1 */
pe_touch_map(touch, PE_TOUCH_EAST, BOTON_DEL_JUEGO);     /* los bits de cada botón: aquí se reasigna */

/* cada cuadro */
PeTouchLayout layout = {ancho, alto, 0, imagen_izq, imagen_der, densidad, borde_izq, borde_der, borde_abajo};
pe_touch_layout(touch, &layout);
botones |= pe_touch_buttons(touch);
if (pe_touch_take_menu(touch)) abrir_menu();

/* eventos de dedo de SDL, en píxeles */
if (pe_touch_finger(touch, PE_FINGER_DOWN, id, x, y) & PE_TOUCH_TAKEN) continue;
```

Los botones van por lugar: `NORTH EAST SOUTH WEST` son X A B Y en SNES y Triángulo Círculo Cruz Cuadrado en PS1. Para dibujarlos:

- **OpenGL ES** (`porteame/touch_gles2.h`): `pe_touch_gl_draw(gl, touch)` dibuja en el framebuffer que esté enlazado y deja el estado de OpenGL como estaba.
- **SDL_Renderer** (`porteame/touch_sdl.h`): `pe_touch_sdl_draw(sdl, touch)` antes de `SDL_RenderPresent`.
- **Otro sistema**: `pe_touch_sprites` da las imágenes listas (RGBA premultiplicado) y dónde van.

## Pruebas del kit (Linux)

```bash
cmake -S . -B build -G Ninja -DPORTEAME_TESTS=ON -DSDL3_DIR=<SDL3 de Linux>/lib/cmake/SDL3 \
      -DPORTEAME_GLES_INCLUDE_DIR=<SDL>/src/video/khronos
cmake --build build && ctest --test-dir build --output-on-failure
```

Prueban la lógica de los controles, las utilidades de iOS (sus caminos de iOS, compilados en Linux), el empaquetador, y dibujan los controles con OpenGL ES y con SDL_Renderer en imágenes `build/tests/touch-*.bmp` para revisarlas a ojo.
