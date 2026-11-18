# Changelog

## v0.1.0

The first kit, from what the Yu-Gi-Oh! Forbidden Memories and EarthBound ports already proved.

- `tools/package_ipa.py`: the unsigned IPA from a `porteame.json` sheet (the same IPA the two ports' own packagers made).
- `tools/ios_env.sh`: SDL3 3.4.16 for iOS (pinned, checked), CMake options for iOS arm64, the executable's check, packaging.
- `actions/ios`: the GitHub Action every fork calls; `templates/` for a new game.
- `tools/port_eame_private.py`: the encrypted personal icon.
- `porteame/ios.h`: startup (Documents, log, environment.txt, landscape), finding the game, safe area, background, errors, and really closing the app on iOS.
- `porteame/touch.h`: the touch controller (SNES and PS1 layouts, remappable buttons, monochrome glass), drawn with OpenGL ES (`touch_gles2.h`) or SDL_Renderer (`touch_sdl.h`).
- Tests on Linux, and the documents in Spanish and English.
