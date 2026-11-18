# Lessons

[Español](../es/lecciones.md)

Every problem already solved in a port, with its cause and its fix. A new port is checked against this list before its first IPA. When a new problem shows up, it is added here.

## Startup and building

**The app doesn't open, or stays black, on iOS 27.**
Cause: SDL2 lacks the scene life cycle (UIScene) the iOS 27 SDK requires, and static sdl2-compat only exists for Linux.
Fix: static SDL3 (3.4.16), and the scene with `SDLUIKitSceneDelegate` in the Info.plist. EarthBound moved straight to SDL3.

**`main` on iOS.**
Fix: `SDL_main.h` in the file that has `main`, `SDL_MAIN_NOIMPL` in the other files that include it, and `SDL_MAIN_HANDLED` outside iOS.

**CMake fails installing the iOS bundle.**
Fix: `-DCMAKE_MACOSX_BUNDLE=OFF` (the kit assembles the `.app`), `-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`, and the frameworks SDL3 needs are read from `sdl3.pc`.

**The game starts in portrait.**
Cause: SDL takes a resizable window for "any orientation", and the Info.plist is not read when another app hosts this one (LiveContainer).
Fix: only the landscape orientations in the Info.plist; before the window is created, SDL's orientation hint set to landscape, and full screen. In the kit: `pe_ios_startup` and `pe_ios_window_flags`.

## Screen and graphics

**The controls sit under the Dynamic Island or the notch.**
Fix: read the window's safe area (it comes in points), turn it into pixels with the display density, and lay out the controls inside it.

**Black screen with OpenGL (errors 0x0502, "composition failed").**
Cause: there is no framebuffer 0 on iOS. SDL draws into its own framebuffer and presents whichever renderbuffer is bound.
Fix: after every effect, bind SDL's framebuffer and renderbuffer again, never 0.

**An OpenGL error blames the wrong step.**
Cause: OpenGL keeps errors until someone reads them.
Fix: read and log the pending errors before every important step, and put the error's code in the message.

**GPU errors on leaving the app.**
Cause: iOS refuses GPU work in the background.
Fix: pause the game loop until the app is back in front.

**The game uses desktop OpenGL (glBegin, alpha test).**
Cause: OpenGL ES doesn't have those old functions.
Fix used in EarthBound: rewrite them by hand (a simple shader, `discard`, vertex arrays). For the next game: gl4es, with no rewrite.

## Saves, settings and closing

**Settings and saves get lost.**
Cause: on iOS the app almost never closes "normally"; the system suspends it and later ends it without warning.
Fix: save when going to the background, and also every so often.

**Closing leaves a black screen and you have to close it again.**
Cause: on iOS, SDL3 doesn't end the app when `main` returns (a comment in its code says so).
Fix: save, then end the process with `exit()`. Yu-Gi-Oh! already does this through the game's own `exit()`. In the kit: `return pe_ios_finish(status);` at the end of `main`. **EarthBound gets it when it moves to the kit.**

**There was no way to quit, and quick saves asked for a name.**
Fix: a "Save and close" button in the menu, and automatic names with the date.

**Settings from another version leave the app black.**
Seen in Yu-Gi-Oh! after restoring a backed-up `settings.txt`; fixed by entering the settings again by hand.
Rule for the kit: validate every setting, ignore unknown ones and, if anything fails, fall back to the defaults.

## Files and log

**The player can't find where to put the game.**
Fix: `UIFileSharingEnabled` and `LSSupportsOpeningDocumentsInPlace` in the Info.plist, so the folder shows in Files. Look for the game there, and show "Importing…" on the first launch.

**There is no way to know what happened.**
Fix: send the program's output (stdout and stderr, unbuffered) to `port-eame-log.txt` in the app's folder.

## Controls

**Buttons don't do what the player expects.**
Cause: every game has its habits; in EarthBound, talking is the SNES L button.
Fix: remap per game. In EarthBound, the on-screen A sends L (talk) and Y sends A (command menu).

## GitHub Actions

**Jobs get cancelled ("not acquired by Runner").**
Cause: little Apple Silicon Mac capacity, and several jobs at once.
Fix: Actions with the manual button only; an Intel Mac option (`macos-15-intel`); the owner runs one job at a time.

**The "Run workflow" button is missing.**
Cause: the workflow has to be on the default branch.
Fix: the fork's default branch is `port-eame/ios`.

## Icon

**The icon or the IPA check fails.**
Cause: `sips` doesn't accept `hasAlpha`, and the icon's work folder ended up inside the IPA.
Fix: copy the 1024 PNG as is, let `actool` build the catalog, and work outside `Payload`.

**A personal icon in a public repository.**
Fix: keep it encrypted (PBKDF2 and HMAC). The Action decrypts it with the `PORTEAME_ICON_KEY` secret only when the icon box is ticked.

## Testing without an iPhone

**Check the graphics before spending an IPA.**
Fix: on Linux, SDL3 in offscreen mode with Mesa (llvmpipe) runs the video tests and simulates iOS's own framebuffer.

## Building without a Mac (pending)

Reviewed: xtool, omarchy-ios-kit (Swift only), buildonce.dev and nix-apple-dev. The most promising for the Linux server is nix-apple-dev, in Docker with the official NixOS image; it needs the Xcode installer.
