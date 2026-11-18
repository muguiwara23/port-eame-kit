#!/usr/bin/env python3
"""Port/eame kit: package a game's iOS executable as an unsigned .ipa.

    package_ipa.py --config porteame.json --binary PATH --out DIR
                   [--version 0.5-ios-dev.12] [--icon icon.png] [--extra DIR]

The game's sheet (porteame.json, docs/es/uso.md) says what goes in the app:
its names, its bundle id, its license notices and any other files. This
writes Payload/<name>.app (the executable, Info.plist, PkgInfo, the license
notices, the files), zips it as an .ipa with a .sha256 beside it, and then
checks the .ipa as an installer reads it. Nothing is signed: the player's own
tool (AltStore, SideStore, FlareStore, LiveContainer) signs it.

The app never holds the game's data: the player puts their own copy in the
app's Documents folder with the Files app. The check refuses an .ipa with a
file of a forbidden extension (ROMs, disc images, packs, saves).

--extra DIR copies a folder the game's build prepared into the app as is
(Yu-Gi-Oh!'s mods); its .dylib files keep no signature and are executable.

Paths in the sheet are relative to the sheet's folder and may name
environment variables, as ${PORTEAME_SDL3_SOURCE}/LICENSE.txt.

Standard library only. From the packagers of Port/eame's Yu-Gi-Oh! Forbidden
Memories and EarthBound ports."""
import argparse
import hashlib
import json
import os
import plistlib
import re
import shutil
import stat
import subprocess
import sys
import zipfile
from pathlib import Path

SHEET_VERSION = 1
# Never in an .ipa, whatever the sheet says: game data and saves.
ALWAYS_FORBIDDEN = (".sfc", ".smc", ".bin", ".cue", ".iso", ".chd", ".z64", ".n64", ".v64", ".nsp", ".xci",
                    ".srm", ".sav", ".state")
DEFAULT_MINIMUM_IOS = "16.0"
LANDSCAPE = ["UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"]


def fail(message):
    raise SystemExit(f"package_ipa: {message}")


def load_sheet(path):
    try:
        sheet = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        fail(f"cannot read {path}: {error}")
    if sheet.get("porteame") != SHEET_VERSION:
        fail(f'{path}: "porteame" must be {SHEET_VERSION} (the sheet\'s format)')
    app = sheet.get("app")
    if not isinstance(app, dict):
        fail(f'{path}: no "app" section')
    for key in ("name", "executable", "bundle_id"):
        if not isinstance(app.get(key), str) or not app[key]:
            fail(f'{path}: "app.{key}" is missing')
    if not re.fullmatch(r"[A-Za-z0-9.-]+", app["bundle_id"]):
        fail(f'{path}: "app.bundle_id" may hold only letters, digits, dots and hyphens')
    if "/" in app["name"] or "/" in app["executable"]:
        fail(f'{path}: "app.name" and "app.executable" are names, not paths')
    return sheet


def resolve(root, text):
    expanded = os.path.expandvars(text)
    if "$" in expanded:
        fail(f"{text}: an environment variable it names is not set")
    path = Path(expanded)
    return path if path.is_absolute() else root / path


def info_plist(app, version, build_number):
    """The Info.plist every Port/eame app has (docs/es/contrato.md)."""
    return dict(
        CFBundleDevelopmentRegion="en",
        CFBundleName=app["name"],
        CFBundleDisplayName=app.get("display_name") or app["name"],
        CFBundleIdentifier=app["bundle_id"],
        CFBundleExecutable=app["executable"],
        CFBundlePackageType="APPL",
        CFBundleInfoDictionaryVersion="6.0",
        CFBundleShortVersionString=version,
        CFBundleVersion=build_number,
        CFBundleSupportedPlatforms=["iPhoneOS"],
        LSRequiresIPhoneOS=True,
        MinimumOSVersion=app.get("minimum_ios", DEFAULT_MINIMUM_IOS),
        UIDeviceFamily=[1, 2],
        UIRequiredDeviceCapabilities=["arm64"],
        # Landscape only, left or right (the game also asks SDL: ios.c).
        UISupportedInterfaceOrientations=LANDSCAPE,
        **{"UISupportedInterfaceOrientations~ipad": LANDSCAPE},
        UIRequiresFullScreen=True,
        UIStatusBarHidden=True,
        UIViewControllerBasedStatusBarAppearance=False,
        # An empty launch screen makes the app use the whole display.
        UILaunchScreen={},
        # The scene life cycle iOS 27 requires; SDL 3.4's UIKit backend
        # supplies this delegate (SDL_uikitappdelegate.m).
        UIApplicationSceneManifest=dict(
            UIApplicationSupportsMultipleScenes=False,
            UISceneConfigurations=dict(
                UIWindowSceneSessionRoleApplication=[
                    dict(
                        UISceneConfigurationName="SDLSceneConfiguration",
                        UISceneDelegateClassName="SDLUIKitSceneDelegate",
                    )
                ]
            ),
        ),
        # Documents shows in the Files app: the player's game, saves, the log.
        UIFileSharingEnabled=True,
        LSSupportsOpeningDocumentsInPlace=True,
        # A Bluetooth mouse or trackpad on iPad is a pointer, not a touch.
        UIApplicationSupportsIndirectInputEvents=True,
        GCSupportsControllerUserInteraction=True,
        ITSAppUsesNonExemptEncryption=False,
    )


def stage_icon(app_dir, icon, info, minimum_ios):
    """The app's icon from one square picture: actool makes the asset
    catalog and the sizes iPhone and iPad want (Info.plist's CFBundleIcons);
    without actool, sips makes the sizes and the plist names them."""
    # Beside the stage, not in it: only Payload/<app> goes into the .ipa.
    work = app_dir.parent.parent.parent / "icon-work"
    shutil.rmtree(work, ignore_errors=True)
    catalog = work / "Assets.xcassets"
    appicon = catalog / "AppIcon.appiconset"
    appicon.mkdir(parents=True)
    # A 1024 x 1024 PNG is used as is (sips cannot be asked about alpha);
    # sips only converts or resizes what is not one.
    picture = appicon / "icon.png"
    shutil.copyfile(icon, picture)
    if icon.suffix.lower() != ".png" or icon.read_bytes()[16:24] != (1024).to_bytes(4, "big") * 2:
        subprocess.run(["sips", "-s", "format", "png", "-z", "1024", "1024", str(icon), "--out", str(picture)],
                       check=True, stdout=subprocess.DEVNULL)
    (catalog / "Contents.json").write_text('{"info":{"author":"xcode","version":1}}')
    (appicon / "Contents.json").write_text(json.dumps({
        "images": [{"filename": "icon.png", "idiom": "universal", "platform": "ios", "size": "1024x1024"}],
        "info": {"author": "xcode", "version": 1},
    }))
    partial = work / "partial.plist"
    made = subprocess.run(
        ["xcrun", "actool", str(catalog), "--compile", str(app_dir), "--platform", "iphoneos",
         "--minimum-deployment-target", minimum_ios, "--app-icon", "AppIcon",
         "--target-device", "iphone", "--target-device", "ipad",
         "--output-partial-info-plist", str(partial), "--output-format", "human-readable-text"],
        check=False,
    ) if shutil.which("xcrun") else None
    if made is not None and made.returncode == 0 and partial.is_file():
        info.update(plistlib.loads(partial.read_bytes()))
        print("icon: asset catalog by actool:",
              sorted(p.name for p in app_dir.glob("AppIcon*")) + (["Assets.car"] if (app_dir / "Assets.car").is_file() else []))
        return
    if not shutil.which("sips"):
        fail("an icon needs Xcode's actool or macOS's sips")
    print("icon: actool failed; plain PNG icons")
    sizes = {"AppIcon60x60@2x.png": 120, "AppIcon60x60@3x.png": 180,
             "AppIcon76x76@2x~ipad.png": 152, "AppIcon83.5x83.5@2x~ipad.png": 167}
    for name, size in sizes.items():
        subprocess.run(["sips", "-z", str(size), str(size), str(picture), "--out", str(app_dir / name)],
                       check=True, stdout=subprocess.DEVNULL)
    info["CFBundleIcons"] = {"CFBundlePrimaryIcon": {"CFBundleIconFiles": ["AppIcon60x60"]}}
    info["CFBundleIcons~ipad"] = {"CFBundlePrimaryIcon": {"CFBundleIconFiles": ["AppIcon60x60", "AppIcon76x76", "AppIcon83.5x83.5"]}}


def remove_signature(path):
    """No signature at all: a stale or ad hoc one only confuses signing tools."""
    if shutil.which("codesign"):
        subprocess.run(["codesign", "--remove-signature", str(path)], check=False)


def short_version(text, fallback):
    match = re.match(r"v?(\d+(?:\.\d+){0,2})", text or "")
    return match.group(1) if match else fallback


def build_number(root, given):
    if given:
        return given
    revision = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root, capture_output=True, text=True).stdout.strip()
    return str(int(revision[:7], 16)) if re.fullmatch(r"[0-9a-f]{7,}", revision) else "1"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
                                     allow_abbrev=False)
    parser.add_argument("--config", type=Path, required=True, help="the game's porteame.json")
    parser.add_argument("--binary", type=Path, required=True, help="the linked iOS executable")
    parser.add_argument("--out", type=Path, required=True, help="where the .ipa goes")
    parser.add_argument("--version", default="", help="the version in the .ipa's name; its leading numbers are the app's")
    parser.add_argument("--build-number", default="", help="CFBundleVersion (default: from the git revision)")
    parser.add_argument("--icon", type=Path, help="a square picture for the app's icon (kept out of the repository)")
    parser.add_argument("--extra", type=Path, action="append", default=[],
                        help="a folder the build prepared, copied into the app as is")
    options = parser.parse_args(argv)

    config = options.config.resolve()
    root = config.parent
    sheet = load_sheet(config)
    app = sheet["app"]
    binary = options.binary.resolve()
    if not binary.is_file():
        fail(f"no executable at {binary}: build it first")
    version_text = options.version or app.get("version", "0.0.0")
    version = short_version(version_text, short_version(app.get("version", ""), "0.0.0"))
    number = build_number(root, options.build_number)
    forbidden = tuple(sorted(set(ALWAYS_FORBIDDEN) | {e.lower() for e in sheet.get("forbidden", [])}))

    out = options.out.resolve()
    stage = out / "stage"
    shutil.rmtree(stage, ignore_errors=True)
    app_dir = stage / "Payload" / f"{app['name']}.app"
    app_dir.mkdir(parents=True)
    executable = app_dir / app["executable"]
    shutil.copyfile(binary, executable)
    executable.chmod(0o755)
    remove_signature(executable)
    (app_dir / "PkgInfo").write_text("APPL????")
    executables = {executable}

    # License notices, in the folder the sheet names (default "licenses").
    licenses = app_dir / sheet.get("licenses_folder", "licenses")
    for name, source in sheet.get("licenses", {}).items():
        path = resolve(root, source)
        if not path.is_file():
            fail(f"missing license notice {name}: {path}")
        licenses.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, licenses / name)
    # Other files: "where in the app": "source file or folder".
    for name, source in sheet.get("files", {}).items():
        path, target = resolve(root, source), app_dir / name
        if path.is_dir():
            shutil.copytree(path, target, dirs_exist_ok=True)
        elif path.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
        else:
            fail(f"missing file {name}: {path}")
    for extra in options.extra:
        if not extra.is_dir():
            fail(f"--extra {extra} is not a folder")
        shutil.copytree(extra, app_dir, dirs_exist_ok=True)
    for library in app_dir.rglob("*.dylib"):
        library.chmod(0o755)
        remove_signature(library)
        executables.add(library)

    info = info_plist(app, version, number)
    if options.icon:
        stage_icon(app_dir, options.icon.resolve(), info, info["MinimumOSVersion"])
    # The sheet's own keys last: they add to or change the defaults.
    info.update(sheet.get("plist", {}))
    (app_dir / "Info.plist").write_bytes(plistlib.dumps(info, fmt=plistlib.FMT_BINARY))

    pattern = sheet.get("ipa", "{executable}-{version}-ios-arm64-unsigned.ipa")
    archive = out / pattern.format(executable=app["executable"], version=version_text, bundle_id=app["bundle_id"])
    archive.unlink(missing_ok=True)
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for path in sorted(stage.rglob("*")):
            if path.is_dir():
                continue
            entry = zipfile.ZipInfo.from_file(path, path.relative_to(stage).as_posix())
            mode = 0o755 if path in executables else 0o644
            entry.external_attr = (stat.S_IFREG | mode) << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            bundle.writestr(entry, path.read_bytes())

    # Check the archive as an installer reads it.
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        prefix = f"Payload/{app['name']}.app/"
        if not all(name.startswith(prefix) for name in names):
            fail("the .ipa holds files outside Payload/<app>")
        read = plistlib.loads(bundle.read(prefix + "Info.plist"))
        if read["CFBundleExecutable"] != app["executable"] or prefix + app["executable"] not in names:
            fail("the .ipa's Info.plist does not name its executable")
        if hashlib.sha256(bundle.read(prefix + app["executable"])).digest() != hashlib.sha256(executable.read_bytes()).digest():
            fail("the .ipa's executable differs from the staged one")
        if not read.get("UIApplicationSceneManifest") or read.get("UIFileSharingEnabled") is not True:
            fail("the .ipa's Info.plist lost the scene manifest or the Files folder")
        found = [n for n in names if n.lower().endswith(forbidden)]
        if found:
            fail(f"the .ipa must hold no game data or saves: {found}")
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    Path(str(archive) + ".sha256").write_text(f"{checksum}  {archive.name}\n")
    print(f"{archive} ({archive.stat().st_size} bytes, {len(names)} files)")
    print(f"sha256 {checksum}")
    print(f"bundle id {app['bundle_id']}, version {version} ({number}), iOS {read['MinimumOSVersion']}+, unsigned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
