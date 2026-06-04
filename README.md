# Suika Launcher

Suika Launcher is a modified fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) with Unified Pass/Nide8 authentication support for Minecraft server distributions.

This project is **not Prism Launcher** and is not endorsed by or affiliated with the Prism Launcher project.

## Current Changes

- Adds a Unified Pass account type and login dialog.
- Supports server ID based authentication against `https://auth.mc-user.com:233/<serverId>/`.
- Defaults the server ID to `aa9441c0487a11e88feb525400b59b6a` while still allowing it to be changed in the Unified Pass login dialog.
- Defaults to simplified Chinese (`zh`) when no language was selected and downloads translations into Suika Launcher's own data directory.
- Implements authenticate, validate, refresh and profile lookup flows compatible with the Mojang-style auth protocol used by Unified Pass.
- Adds launch-time JVM arguments for Unified Pass sessions:
  - `-javaagent:<nide8auth.jar path>=<serverId>`
  - `-Dnide8auth.client=true`
- Checks that the selected Java runtime is Java 8 update 101 or newer before launching a Unified Pass session.
- Passes Unified Pass accounts to Minecraft as Mojang/Yggdrasil-compatible sessions and loads the Nide8 agent so Minecraft 1.19+ secure profile public-key checks use the Unified Pass server metadata.
- Clears Prism Launcher's bundled Microsoft, CurseForge and Imgur API keys for derivative-build compliance.
- Rebrands build metadata and package identity to Suika Launcher.

Suika Launcher uses its own application data directory. It does not automatically reuse Prism Launcher's instances, accounts, assets, libraries or configuration files.

## Runtime Agent

Unified Pass launch requires `nide8auth.jar`.

The source tree includes an authorized runtime copy at `third_party/nide8auth/nide8auth.jar`. CMake copies and installs it next to the launcher executable on macOS, Windows and Linux. The jar is an independent third-party runtime component provided by nide8/Unified Pass and is documented in [NOTICE.md](NOTICE.md) and [third_party/nide8auth/README.md](third_party/nide8auth/README.md).

## Building

The upstream Prism Launcher build system is preserved, so the project remains CMake/Qt based and can target macOS, Windows and Linux.

Typical build requirements are the same as Prism Launcher:

- CMake
- Ninja or another supported CMake generator
- Qt 6
- Java 17 for building the helper jars
- vcpkg dependencies used by Prism Launcher

Platform packages should set a distinct build platform, for example:

```sh
cmake --preset macos \
  -DLauncher_BUILD_PLATFORM=suika-macos \
  -DLauncher_BUILD_ARTIFACT=SuikaLauncher-macOS
```

## API Keys

This fork intentionally ships with Prism's bundled third-party API keys removed.

If you want Microsoft account login, CurseForge integration or Imgur uploads in a public build, register your own keys and pass them through CMake:

```sh
-DLauncher_MSA_CLIENT_ID=...
-DLauncher_CURSEFORGE_API_KEY=...
-DLauncher_IMGUR_CLIENT_ID=...
```

## License

Launcher code is licensed under GPL-3.0-only. See [LICENSE](LICENSE).

This fork incorporates Prism Launcher, PolyMC and MultiMC work. See [COPYING.md](COPYING.md) and [FORK_NOTICE.md](FORK_NOTICE.md) for attribution and modification notes.

Logo and related inherited assets are licensed under CC BY-SA 4.0. See [program_info/LICENSE](program_info/LICENSE).
