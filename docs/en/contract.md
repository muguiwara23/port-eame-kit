# The Port/eame contract

[Español](../es/contrato.md)

What every Port/eame app does the same way, whatever the game. A port is ready when it meets every point. The kit handles most of them; the game only hooks up its own part.

## 1. Startup

- Opens in landscape (left or right), full screen, with no status bar.
- Uses the scene life cycle (UIScene) that iOS 27 requires. That is why the kit uses SDL3.
- Respects the safe area: nothing important sits under the Dynamic Island, the notch or the home bar.
- Runs on 64-bit iPhones and iPads with iOS 16 or newer.

## 2. The player's files

- The app's folder shows up in the Files app.
- The game (ROM, disc or pack) is looked for there. If it is missing, the app says clearly which file to put where, never with a black screen.
- If the game needs to import or extract data, it does so by itself on the first launch and shows that it is working.
- Saves, settings and the log live in that same folder, so the player can back them up.

## 3. Controls

- Touch controls with the original console's layout (PS1, SNES…), in Port/eame's monochrome glass style.
- A MENU button opens the port's options.
- Bluetooth gamepads work with no setup.
- Each game can remap buttons when the game's habits call for it (in EarthBound, A talks directly).

## 4. Options

- The port's menu opens with MENU and is used by touch.
- Settings are saved when changed and survive updates: an old or unknown setting is ignored, it doesn't break the app.

## 5. Background

- On leaving the app: pauses the game, the audio and the GPU (iOS doesn't allow drawing in the background), and saves the game and the settings.
- On coming back: carries on where it was.
- It also saves every so often, because iOS can close the app without warning.

## 6. Closing

- "Save and close" saves and really ends the app. It never leaves a black screen.

## 7. Log and errors

- Everything the port prints goes to `port-eame-log.txt` in the app's folder, to share when something fails.
- An error is shown on screen with a clear message, as well as written to the log.

## 8. Graphics

- Base: OpenGL ES 2.0/3.0, tested on iOS 27.
- If the game uses another graphics system, an existing translator is used, not a rewrite by hand. See [Routes](routes.md).

## 9. The IPA

- Unsigned: the player's own tool signs it (AltStore, SideStore, FlareStore, LiveContainer).
- ID `org.porteame.<game>`, with every component's license inside.
- An automatic check fails if the IPA holds a ROM, a disc, a data pack or a save.
- Optional personal icon, kept encrypted in the repository. The public build ships without it.

## 10. Building

- GitHub Actions, with the manual button only ("Run workflow"). Intel or Apple Silicon Mac to choose.
- The project owner runs it, one job at a time.
- Later, also on a Linux server.

## What each game provides

- **The `porteame.json` sheet:** name, ID, version, executable, licenses, files and forbidden extensions.
- **A small adapter in its code:** which controller (SNES or PS1), what each button sends, where it saves and how it pauses.

How, in [How to use the kit](usage.md).
