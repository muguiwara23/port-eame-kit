# Contrato de Port/eame

[English](../en/contract.md)

Lo que toda app de Port/eame hace igual, sea el juego que sea. Un port está listo cuando cumple cada punto. El kit se encarga de la mayoría; el juego solo conecta lo suyo.

## 1. Arranque

- Abre en horizontal (izquierda o derecha), en pantalla completa y sin barra de estado.
- Usa el ciclo de escenas (UIScene) que exige iOS 27. Por eso el kit usa SDL3.
- Respeta los bordes seguros: nada importante queda bajo la isla, el notch o la barra de inicio.
- Funciona en iPhone y iPad de 64 bits con iOS 16 o más nuevo.

## 2. Los archivos del jugador

- La carpeta de la app aparece en la app Archivos.
- El juego (ROM, disco o paquete) se busca ahí. Si falta, la app dice con claridad qué archivo poner y dónde, nunca con una pantalla negra.
- Si el juego necesita importar o extraer datos, lo hace solo en el primer arranque y muestra que está trabajando.
- Partidas, ajustes y registro viven en esa misma carpeta, para que el jugador pueda respaldarlos.

## 3. Controles

- Controles táctiles con la distribución de la consola original (PS1, SNES…), en el estilo de vidrio monocromo de Port/eame.
- Un botón MENU abre las opciones del port.
- Los mandos Bluetooth funcionan sin configurar nada.
- Cada juego puede reasignar botones cuando la costumbre del juego lo pide (en EarthBound, A habla directo).

## 4. Opciones

- El menú del port se abre con MENU y se usa con el dedo.
- Los ajustes se guardan al cambiarlos y sobreviven a las actualizaciones: un ajuste viejo o desconocido se ignora, no rompe la app.

## 5. Segundo plano

- Al salir de la app: pausa el juego, el audio y la GPU (iOS no deja dibujar en segundo plano), y guarda la partida y los ajustes.
- Al volver: continúa donde estaba.
- Además guarda cada cierto tiempo, porque iOS puede cerrar la app sin avisar.

## 6. Cerrar

- "Guardar y cerrar" guarda y termina la app de verdad. Nunca deja una pantalla negra.

## 7. Registro y errores

- Todo lo que el port escribe va a `port-eame-log.txt` en la carpeta de la app, para compartirlo cuando algo falla.
- Un error se muestra en pantalla con un mensaje claro, además de quedar en el registro.

## 8. Gráficos

- Base: OpenGL ES 2.0/3.0, probado en iOS 27.
- Si el juego usa otro sistema gráfico, se usa un traductor que ya existe, no una reescritura a mano. Ver [Rutas](rutas.md).

## 9. El IPA

- Sin firmar: lo firma la herramienta del jugador (AltStore, SideStore, FlareStore, LiveContainer).
- ID `org.porteame.<juego>`, con las licencias de cada componente dentro.
- Una revisión automática falla si el IPA trae un ROM, un disco, un paquete de datos o una partida.
- Ícono personal opcional, guardado cifrado en el repositorio. La versión pública va sin él.

## 10. Compilación

- GitHub Actions, solo con el botón manual ("Run workflow"). Se elige Mac Intel o Apple Silicon.
- Lo corre el dueño del proyecto, un trabajo a la vez.
- Más adelante, también en un servidor Linux.

## Lo que pone cada juego

- **La ficha `porteame.json`:** nombre, ID, versión, ejecutable, licencias, archivos y extensiones prohibidas.
- **Un adaptador pequeño en su código:** qué control (SNES o PS1), qué manda cada botón, dónde guarda y cómo se pausa.

Cómo se hace, en [Cómo usar el kit](uso.md).
