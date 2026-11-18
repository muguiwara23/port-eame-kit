# Lecciones

[English](../en/lessons.md)

Cada problema que ya resolvimos en un port, con su causa y su solución. Un port nuevo se revisa contra esta lista antes del primer IPA. Cuando aparece un problema nuevo, se agrega aquí.

## Arranque y compilación

**La app no abre o se queda en negro en iOS 27.**
Causa: SDL2 no tiene el ciclo de escenas (UIScene) que exige el SDK de iOS 27, y sdl2-compat estático solo existe para Linux.
Solución: SDL3 (3.4.16) estático, y en el Info.plist la escena con `SDLUIKitSceneDelegate`. EarthBound se pasó directo a SDL3.

**`main` en iOS.**
Solución: `SDL_main.h` en el archivo que tiene `main`, `SDL_MAIN_NOIMPL` en los demás archivos que lo incluyan, y `SDL_MAIN_HANDLED` fuera de iOS.

**CMake falla al instalar el bundle de iOS.**
Solución: `-DCMAKE_MACOSX_BUNDLE=OFF` (el kit arma el `.app`), `-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`, y los frameworks que pide SDL3 se leen de `sdl3.pc`.

**El juego arranca en vertical.**
Solución: en el Info.plist, solo las orientaciones horizontales; antes de crear la ventana, la pista de orientación de SDL en horizontal, y pantalla completa.

## Pantalla y gráficos

**Los controles quedan bajo la isla o el notch.**
Solución: leer el área segura de la ventana (viene en puntos), convertirla a píxeles con la densidad de pantalla y acomodar los controles dentro.

**Pantalla negra con OpenGL (errores 0x0502, "composition failed").**
Causa: en iOS no existe el framebuffer 0. SDL dibuja en su propio framebuffer y presenta el renderbuffer que esté enlazado.
Solución: después de cada efecto, volver a enlazar el framebuffer y el renderbuffer de SDL, nunca el 0.

**Un error de OpenGL culpa al paso equivocado.**
Causa: OpenGL guarda los errores hasta que alguien los lee.
Solución: leer y registrar los errores pendientes antes de cada paso importante, y poner el código del error en el mensaje.

**Errores de GPU al salir de la app.**
Causa: iOS rechaza el trabajo de GPU en segundo plano.
Solución: pausar el bucle del juego hasta que la app vuelva al frente.

**El juego usa OpenGL de PC (glBegin, alpha test).**
Causa: OpenGL ES no tiene esas funciones viejas.
Solución usada en EarthBound: reescribirlas a mano (un shader simple, `discard`, arreglos de vértices). Para el próximo juego: gl4es, sin reescribir.

## Partidas, ajustes y cierre

**Los ajustes y las partidas se pierden.**
Causa: en iOS la app casi nunca se cierra "normal"; el sistema la suspende y luego la termina sin avisar.
Solución: guardar al pasar a segundo plano y además cada cierto tiempo.

**Al cerrar queda una pantalla negra y hay que cerrar otra vez.**
Causa: en iOS, SDL3 no termina la app cuando `main` regresa (lo dice un comentario en su código).
Solución: guardar y luego terminar el proceso con `exit()`. Yu-Gi-Oh! ya lo hace con el `exit()` del juego. **En EarthBound queda pendiente.**

**No había forma de salir, y los guardados rápidos pedían nombre.**
Solución: botón "Guardar y cerrar" en el menú, y nombres automáticos con la fecha.

**Unos ajustes de otra versión dejan la app en negro.**
Visto en Yu-Gi-Oh! al restaurar un `settings.txt` respaldado; se arregló volviendo a poner los ajustes a mano.
Regla para el kit: validar cada ajuste, ignorar los desconocidos y, si algo falla, volver a los valores por defecto.

## Archivos y registro

**El jugador no encuentra dónde poner el juego.**
Solución: `UIFileSharingEnabled` y `LSSupportsOpeningDocumentsInPlace` en el Info.plist, para que la carpeta se vea en Archivos. Buscar el juego ahí, y mostrar "Importando…" en el primer arranque.

**No hay forma de saber qué pasó.**
Solución: mandar la salida del programa (stdout y stderr, sin búfer) a `port-eame-log.txt` en la carpeta de la app.

## Controles

**Los botones no hacen lo que el jugador espera.**
Causa: cada juego tiene sus costumbres; en EarthBound, hablar es el botón L del SNES.
Solución: reasignar por juego. En EarthBound, la A en pantalla manda L (hablar) y la Y manda A (menú de comandos).

## GitHub Actions

**Los trabajos se cancelan ("not acquired by Runner").**
Causa: poca capacidad de Macs Apple Silicon, y varios trabajos a la vez.
Solución: Actions solo con botón manual; opción de Mac Intel (`macos-15-intel`); el dueño corre un trabajo a la vez.

**No aparece el botón "Run workflow".**
Causa: el workflow tiene que estar en la rama por defecto.
Solución: la rama por defecto del fork es `port-eame/ios`.

## Ícono

**Falla el ícono o la revisión del IPA.**
Causa: `sips` no acepta `hasAlpha`, y la carpeta de trabajo del ícono quedaba dentro del IPA.
Solución: copiar el PNG de 1024 tal cual, dejar que `actool` arme el catálogo, y trabajar fuera de `Payload`.

**El ícono personal en un repositorio público.**
Solución: guardarlo cifrado (PBKDF2 y HMAC). El Action lo descifra con el secreto `PORTEAME_ICON_KEY` solo si se marca la casilla del ícono.

## Probar sin iPhone

**Revisar los gráficos antes de gastar un IPA.**
Solución: en Linux, SDL3 con el modo sin pantalla y Mesa (llvmpipe) corre las pruebas de video y simula el framebuffer propio de iOS.

## Compilar fuera de una Mac (pendiente)

Revisadas: xtool, omarchy-ios-kit (solo Swift), buildonce.dev y nix-apple-dev. La más prometedora para el servidor Linux es nix-apple-dev, en Docker con la imagen oficial de NixOS; necesita el instalador de Xcode.
