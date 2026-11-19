#!/usr/bin/env python3
"""Port/eame kit: package_ipa.py on a made-up game, on any system."""
import json
import os
import plistlib
import stat
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

TOOL = Path(__file__).resolve().parents[1] / "tools" / "package_ipa.py"


def run(*args, ok=True):
    done = subprocess.run([sys.executable, str(TOOL), *map(str, args)], capture_output=True, text=True)
    if (done.returncode == 0) != ok:
        sys.exit(f"unexpected result {done.returncode}:\n{done.stdout}\n{done.stderr}")
    return done


def app_mode(scratch):
    """An app Xcode built: kept as it is, renamed, unsigned, checked."""
    root = scratch / "xcode-game"
    built = root / "build" / "JarBox.app"
    (built / "_CodeSignature").mkdir(parents=True)
    (built / "_CodeSignature" / "CodeResources").write_text("sig")
    (built / "embedded.mobileprovision").write_text("profile")
    (built / "JarBox").write_bytes(b"\xcf\xfa\xed\xfe" + os.urandom(64))
    (built / "JarBox").chmod(0o755)
    (built / "Assets.car").write_bytes(b"car")
    (built / "minijvm_rt.jar").write_bytes(b"PK")
    (built / "Info.plist").write_bytes(plistlib.dumps({
        "CFBundleExecutable": "JarBox", "CFBundleIdentifier": "com.example.old", "CFBundleDisplayName": "Old",
        "UIApplicationSceneManifest": {"UIApplicationSupportsMultipleScenes": False},
        "UIFileSharingEnabled": True, "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"],
    }, fmt=plistlib.FMT_BINARY))
    sheet = {"porteame": 1,
             "app": {"name": "Port-eame Java", "display_name": "Port/eame Java", "executable": "JarBox",
                     "bundle_id": "org.porteame.java", "version": "0.1"},
             "ipa": "PortEameJava-{version}-ios-arm64-unsigned.ipa",
             "forbidden": [".jar", ".jad"], "allow": ["minijvm_rt.jar"],
             "plist": {"CFBundleName": "Port-eame Java"}}
    (root / "porteame.json").write_text(json.dumps(sheet))
    out = scratch / "app-out"
    run("--config", root / "porteame.json", "--app", built, "--out", out, "--version", "0.1-ios-dev.3",
        "--build-number", "5")
    with zipfile.ZipFile(out / "PortEameJava-0.1-ios-dev.3-ios-arm64-unsigned.ipa") as bundle:
        names = set(bundle.namelist())
        prefix = "Payload/Port-eame Java.app/"
        assert prefix + "Assets.car" in names and prefix + "minijvm_rt.jar" in names
        assert not any("_CodeSignature" in n or "mobileprovision" in n for n in names), names
        assert (bundle.getinfo(prefix + "JarBox").external_attr >> 16) & stat.S_IXUSR
        info = plistlib.loads(bundle.read(prefix + "Info.plist"))
        assert info["CFBundleIdentifier"] == "org.porteame.java"
        assert info["CFBundleDisplayName"] == "Port/eame Java"
        assert info["CFBundleShortVersionString"] == "0.1" and info["CFBundleVersion"] == "5"
        assert info["UISupportedInterfaceOrientations"] == ["UIInterfaceOrientationPortrait"]  # the app's own
        assert info["CFBundleName"] == "Port-eame Java"
    # A game's .jar in the app is refused; the runtime's own is allowed.
    (built / "SomeGame.jar").write_bytes(b"PK")
    refused = run("--config", root / "porteame.json", "--app", built, "--out", out, ok=False)
    assert "no game data" in refused.stderr and "SomeGame.jar" in refused.stderr, refused.stderr
    (built / "SomeGame.jar").unlink()
    # The sheet must name the built executable.
    sheet["app"]["executable"] = "Other"
    (root / "porteame.json").write_text(json.dumps(sheet))
    refused = run("--config", root / "porteame.json", "--app", built, "--out", out, ok=False)
    assert "executable" in refused.stderr, refused.stderr


def main():
    with tempfile.TemporaryDirectory() as scratch:
        game = Path(scratch) / "game"
        (game / "data").mkdir(parents=True)
        (game / "LICENSE").write_text("MIT\n")
        (game / "data" / "words.txt").write_text("hello\n")
        sdl = Path(scratch) / "sdl"
        sdl.mkdir()
        (sdl / "LICENSE.txt").write_text("zlib\n")
        sheet = {
            "porteame": 1,
            "app": {"name": "Test Game", "display_name": "Test", "executable": "TestGame",
                    "bundle_id": "org.porteame.test", "version": "1.2"},
            "licenses": {"Test-LICENSE.txt": "LICENSE", "SDL3.txt": "${PORTEAME_SDL3_SOURCE}/LICENSE.txt"},
            "files": {"data": "data", "LICENSE.txt": "LICENSE"},
            "forbidden": [".pak"],
            "plist": {"UIFileSharingEnabled": True, "PortEameTest": "yes"},
        }
        (game / "porteame.json").write_text(json.dumps(sheet))
        binary = Path(scratch) / "binary"
        binary.write_bytes(os.urandom(4096))
        extra = Path(scratch) / "extra"
        (extra / "mods" / "one").mkdir(parents=True)
        (extra / "mods" / "one" / "mod.dylib").write_bytes(b"\xcf\xfa\xed\xfe")
        out = Path(scratch) / "out"
        os.environ["PORTEAME_SDL3_SOURCE"] = str(sdl)
        run("--config", game / "porteame.json", "--binary", binary, "--out", out, "--version", "1.2.3-ios-dev.7",
            "--build-number", "42", "--extra", extra)
        archive = out / "TestGame-1.2.3-ios-dev.7-ios-arm64-unsigned.ipa"
        assert archive.is_file(), list(out.iterdir())
        assert (out / (archive.name + ".sha256")).is_file()
        with zipfile.ZipFile(archive) as bundle:
            names = set(bundle.namelist())
            prefix = "Payload/Test Game.app/"
            for name in ("TestGame", "Info.plist", "PkgInfo", "licenses/Test-LICENSE.txt", "licenses/SDL3.txt",
                         "data/words.txt", "LICENSE.txt", "mods/one/mod.dylib"):
                assert prefix + name in names, name
            info = plistlib.loads(bundle.read(prefix + "Info.plist"))
            assert info["CFBundleShortVersionString"] == "1.2.3"
            assert info["CFBundleVersion"] == "42"
            assert info["CFBundleDisplayName"] == "Test"
            assert info["PortEameTest"] == "yes"
            assert info["UISupportedInterfaceOrientations"] == ["UIInterfaceOrientationLandscapeLeft",
                                                                 "UIInterfaceOrientationLandscapeRight"]
            for name in ("TestGame", "mods/one/mod.dylib"):
                mode = bundle.getinfo(prefix + name).external_attr >> 16
                assert mode & stat.S_IXUSR, name
            assert not (bundle.getinfo(prefix + "PkgInfo").external_attr >> 16) & stat.S_IXUSR

        # Game data or a save anywhere in the app is refused.
        (game / "data" / "game.sfc").write_bytes(b"rom")
        refused = run("--config", game / "porteame.json", "--binary", binary, "--out", out, ok=False)
        assert "no game data" in refused.stderr, refused.stderr
        (game / "data" / "game.sfc").unlink()
        (game / "data" / "assets.PAK").write_bytes(b"pack")
        refused = run("--config", game / "porteame.json", "--binary", binary, "--out", out, ok=False)
        assert "no game data" in refused.stderr, refused.stderr
        (game / "data" / "assets.PAK").unlink()

        # A missing license notice or an unset variable stops the packaging.
        del os.environ["PORTEAME_SDL3_SOURCE"]
        refused = run("--config", game / "porteame.json", "--binary", binary, "--out", out, ok=False)
        assert "not set" in refused.stderr, refused.stderr
        os.environ["PORTEAME_SDL3_SOURCE"] = str(sdl)
        (game / "LICENSE").unlink()
        refused = run("--config", game / "porteame.json", "--binary", binary, "--out", out, ok=False)
        assert "missing license" in refused.stderr, refused.stderr

        # A sheet of another format is refused.
        (game / "LICENSE").write_text("MIT\n")
        sheet["porteame"] = 2
        (game / "porteame.json").write_text(json.dumps(sheet))
        refused = run("--config", game / "porteame.json", "--binary", binary, "--out", out, ok=False)
        assert "format" in refused.stderr, refused.stderr
        app_mode(Path(scratch))
    print("package: all good")


if __name__ == "__main__":
    main()
