# Suika Launcher

Suika Launcher is a server-focused fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) for the Suika server / `西瓜幻想乡`.

This project is **not Prism Launcher** and is not endorsed by or affiliated with the Prism Launcher project. Upstream copyright notices, license files and attribution are retained.

## Main Changes From Prism Launcher

### Server-Only Account Flow

- Adds Unified Pass / Nide8 authentication for the Suika server.
- Fixes the Unified Pass server ID to `aa9441c0487a11e88feb525400b59b6a`; players cannot edit it in the login UI.
- Uses the Unified Pass Mojang/Yggdrasil-compatible endpoints for authenticate, validate, refresh and profile lookup.
- Requires Unified Pass accounts to validate online before launching.
- Loads Unified Pass skins through the server sessionserver-compatible profile endpoint.
- Installs and uses `nide8auth.jar` at launch time with:
  - `-javaagent:<nide8auth.jar path>=<serverId>`
  - `-Dnide8auth.client=true`
- Requires Java 8 update 101 or newer for Unified Pass launches.
- Hides Microsoft login, offline login and other account types from the player-facing account UI.

### Suika Server Experience

- Keeps the technical product/package name as `Suika Launcher`, while player-facing UI displays `西瓜幻想乡`.
- Uses `https://www.suika.fun/` for website, help and launcher links.
- Uses an independent Suika Launcher data directory instead of reusing Prism Launcher's data directory.
- Adds the default multiplayer server entry to created/imported instances:
  - Name: `西瓜幻想乡群组服务器`
  - Address: `mc.suika.fun`
- Shows a startup internal-testing notice for current test builds.
- Disables launcher self-update checks.

### Downloads And Mod Sources

- Adds a selectable download source setting:
  - `BMCLAPI 中国下载源`
  - `官方默认源`
- Defaults new installs to BMCLAPI for better download performance in China.
- Rewrites supported Mojang/library/asset/Java runtime/mod-loader Maven downloads to BMCLAPI when that source is selected.
- Falls back to the official source if a BMCLAPI download fails.
- Does not rewrite Unified Pass, Suika website, Microsoft auth, skin texture, Modrinth API/CDN or other non-whitelisted requests.
- Keeps Modrinth mod/resource download support enabled.
- Hides CurseForge/Flame, FTB/ATLauncher/Technic pack browsing and other unused pack download entries from the player-facing flow.
- Keeps vanilla, Fabric and NeoForge instance creation visible; hides Forge, Quilt and LiteLoader in the new-instance UI.

### Language And UI

- Bundles English and Simplified Chinese only.
- Defaults first-run language to Simplified Chinese (`zh_CN`) and supports old `zh` settings.
- Disables remote translation downloads.
- Adds Simplified Chinese text for Suika-specific Unified Pass and launcher UI.
- Removes player-facing settings pages that are not useful for this server build, including proxy and external tools pages.

### Branding And Packaging

- Version is currently `1.1`.
- App/bundle/binary/internal product identity stays ASCII-safe as `Suika Launcher`.
- The player-facing name is `西瓜幻想乡`.
- Replaces the app icon with Suika branding based on `https://www.suika.fun/static/picture/20200507125350.png`.
- Removes bundled Prism third-party API keys and disables the corresponding Microsoft, CurseForge and Imgur integrations.

## Runtime Agent

Unified Pass launch requires `nide8auth.jar`.

The source tree includes a runtime copy at `third_party/nide8auth/nide8auth.jar`. CMake copies and installs it next to the launcher executable on macOS, Windows and Linux. The jar is an independent third-party runtime component provided by nide8/Unified Pass and is documented in [NOTICE.md](NOTICE.md) and [third_party/nide8auth/README.md](third_party/nide8auth/README.md).

## Third-Party Notices

- Launcher code remains GPL-3.0-only. See [LICENSE](LICENSE).
- This fork incorporates Prism Launcher, PolyMC and MultiMC work. See [COPYING.md](COPYING.md) and [FORK_NOTICE.md](FORK_NOTICE.md).
- Logo and inherited assets are licensed under CC BY-SA 4.0. See [program_info/LICENSE](program_info/LICENSE).
- BMCLAPI download mirror support is based on the public BMCLAPI path scheme. See [BMCLAPI documentation](https://bmclapidoc.bangbang93.com/) and [openbmclapi / BMCLAPI@Home](https://github.com/bangbang93/openbmclapi), MIT License.
- `nide8auth.jar` is a separate runtime component and is not authored by Prism Launcher.

## Resource Hosting

Suika Launcher does not host or bundle Minecraft game files, assets, libraries, Java runtimes, mod loader files or Modrinth files. Those files are downloaded from the selected official/BMCLAPI/third-party source at runtime.

## Building

The upstream Prism Launcher build system is preserved, so the project remains CMake/Qt based and can target macOS, Windows and Linux.

Typical build requirements are the same as Prism Launcher:

- CMake
- Ninja or another supported CMake generator
- Qt 6
- Java 17 for building helper jars
- vcpkg dependencies used by Prism Launcher

Example macOS configure command:

```sh
cmake --preset macos \
  -DLauncher_BUILD_PLATFORM=suika-macos \
  -DLauncher_BUILD_ARTIFACT=SuikaLauncher-macOS
```

## API Keys

This server build intentionally ships with Prism's bundled third-party API keys removed and the corresponding Microsoft, CurseForge and Imgur integrations disabled in code.

Do not add Prism Launcher's private third-party API keys back into this fork. Re-enabling those integrations for another public derivative would require your own API keys and code changes that undo the Suika server-only restrictions.

## License

Launcher code is licensed under GPL-3.0-only. See [LICENSE](LICENSE).

This fork retains upstream Prism Launcher, PolyMC and MultiMC attribution. See [COPYING.md](COPYING.md), [FORK_NOTICE.md](FORK_NOTICE.md) and [NOTICE.md](NOTICE.md).
