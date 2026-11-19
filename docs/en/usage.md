# How to use the kit

[Español](../es/uso.md)

What is done once in each game's fork. After that, every new IPA is just running the Action.

## 1. Bring the kit in as a submodule

On the fork's `port-eame/ios` branch:

```bash
git submodule add https://github.com/muguiwara23/port-eame-kit port-eame-kit
git -C port-eame-kit checkout v0.1.0
git add .gitmodules port-eame-kit
```

The kit stays pinned at that version. To move it later: `git -C port-eame-kit checkout v0.2.0` and a commit. That way a new kit version never breaks a game that already works.

## 2. The game's sheet: `porteame.json`

At the fork's root. There is a template in `templates/porteame.json`.

| Field | What it is |
| --- | --- |
| `porteame` | The sheet's format: `1` |
| `app.name` | The `.app`'s name and `CFBundleName` |
| `app.display_name` | The name under the icon (default: `name`) |
| `app.executable` | The executable's name inside the `.app` |
| `app.bundle_id` | `org.porteame.<game>` |
| `app.version` | The game's version, for example `0.5` |
| `app.minimum_ios` | The oldest iOS (default `16.0`) |
| `ipa` | The IPA's name; `{version}` and `{executable}` are filled in |
| `licenses` | `"name in the app": "file"`, into the `licenses` folder |
| `files` | `"where in the app": "file or folder"`, other files the game needs |
| `forbidden` | Extensions that must never go in, beyond the kit's own |
| `icon` | The encrypted icon, for example `ios/private/app-icon.png.enc` |
| `plist` | Info.plist keys to add or change |

Paths are relative to the sheet and may use variables, like `${PORTEAME_SDL3_SOURCE}/LICENSE.txt` (SDL3's license).

The kit always refuses ROMs, disc images and saves (`.sfc .smc .bin .cue .iso .chd .z64 .n64 .v64 .nsp .xci .srm .sav .state`), whatever the sheet says.

## 3. The build script

`ios/build_ios.sh` (template in `templates/build_ios.sh`): sources the kit's `tools/ios_env.sh`, builds SDL3, builds the game with CMake for iOS, checks the executable is iOS arm64 and makes the IPA. The game only changes its CMake options and its executable's name.

**If the project builds with Xcode** (an `.xcodeproj`, like Port/eame Java), the script uses `porteame_xcodebuild project.xcodeproj target` and then `porteame_package_app porteame.json path/App.app`. The app keeps its own Info.plist (orientations, permissions), and the kit sets the sheet's ID, name, version and `plist` keys on it and takes its signatures out. In the sheet, `allow` names files that may go in although their extension is forbidden, like the engine's own `.jar` files.

## 4. The Action

Copy `templates/port-eame-ios.yml` to `.github/workflows/port-eame-ios.yml`. It uses the kit's action (`port-eame-kit/actions/ios`), with the manual button, a choice of Intel or Apple Silicon Mac and the icon box. The icon is decrypted with the repository's `PORTEAME_ICON_KEY` secret.

Remember: the "Run workflow" button only shows when the workflow is on the fork's default branch (`port-eame/ios`). To try another branch, pick it under "Use workflow from".

## 5. The game's code

In the game's `CMakeLists.txt`:

```cmake
add_subdirectory(port-eame-kit)
target_link_libraries(my_game PRIVATE porteame::kit)
```

### iOS helpers: `porteame/ios.h`

| Function | What for |
| --- | --- |
| `pe_ios_startup(name, folders)` | First thing in `main`: Documents, the `port-eame-log.txt` log, `environment.txt`, landscape |
| `pe_ios_find_file(folder, extensions, …)` | Find the player's game in Documents |
| `pe_ios_window_flags()` | Borderless full screen on iOS |
| `pe_ios_density(window)` / `pe_ios_safe_area(…)` | Pixels per point and the safe area, in pixels |
| `pe_ios_is_background(event)` + `pe_ios_wait_for_foreground(save, data)` | Save and pause in the background |
| `pe_ios_error(title, message)` | An error the player sees |
| `return pe_ios_finish(status);` | The end of `main`: on iOS it really ends the app |

Off iOS they do nothing, or the plain thing, so no `#ifdef` is needed.

### Touch controls: `porteame/touch.h`

```c
PeTouch *touch = pe_touch_create(PE_TOUCH_SNES);     /* or PE_TOUCH_PS1 */
pe_touch_map(touch, PE_TOUCH_EAST, GAME_BUTTON);      /* each button's bits: remapping happens here */

/* every frame */
PeTouchLayout layout = {width, height, 0, picture_left, picture_right, density, inset_left, inset_right, inset_bottom};
pe_touch_layout(touch, &layout);
buttons |= pe_touch_buttons(touch);
if (pe_touch_take_menu(touch)) open_menu();

/* SDL finger events, in pixels */
if (pe_touch_finger(touch, PE_FINGER_DOWN, id, x, y) & PE_TOUCH_TAKEN) continue;
```

Buttons go by place: `NORTH EAST SOUTH WEST` are X A B Y on SNES and Triangle Circle Cross Square on PS1. To draw them:

- **OpenGL ES** (`porteame/touch_gles2.h`): `pe_touch_gl_draw(gl, touch)` draws into whichever framebuffer is bound and leaves OpenGL's state as it was.
- **SDL_Renderer** (`porteame/touch_sdl.h`): `pe_touch_sdl_draw(sdl, touch)` before `SDL_RenderPresent`.
- **Anything else**: `pe_touch_sprites` gives the finished pictures (premultiplied RGBA) and where they go.

## The kit's tests (Linux)

```bash
cmake -S . -B build -G Ninja -DPORTEAME_TESTS=ON -DSDL3_DIR=<Linux SDL3>/lib/cmake/SDL3 \
      -DPORTEAME_GLES_INCLUDE_DIR=<SDL>/src/video/khronos
cmake --build build && ctest --test-dir build --output-on-failure
```

They test the controls' logic, the iOS helpers (their iOS paths, built on Linux), the packager, and draw the controls with OpenGL ES and with SDL_Renderer into `build/tests/touch-*.bmp` pictures to check by eye.
