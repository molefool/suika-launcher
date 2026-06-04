# Fork Notice

Suika Launcher is a modified fork of Prism Launcher.

It is not Prism Launcher and is not endorsed by or affiliated with the Prism Launcher project.

## Upstream

- Upstream project: Prism Launcher
- Upstream repository: https://github.com/PrismLauncher/PrismLauncher
- Upstream base commit used for this fork: `6f3574f328f9aeafd4db04da122ef79de5d09c4e`
- Upstream license: GPL-3.0-only for launcher code

## Modification Notice

Date: 2026-06-04

Primary fork changes:

- Added Unified Pass/Nide8 account and authentication support.
- Added server ID and auth base discovery support.
- Added Unified Pass skin/profile lookup support.
- Added JVM argument injection for Unified Pass launches.
- Added Java version validation for Unified Pass launches.
- Added default Unified Pass server ID: `aa9441c0487a11e88feb525400b59b6a`.
- Fixed the Unified Pass server ID for the Suika server and hid the server ID editor from the login UI.
- Added simplified Chinese fallback text for the new Unified Pass account UI.
- Added simplified Chinese as the preferred first-run language and migrated existing Suika Launcher settings to `zh_CN` once.
- Bundled English and simplified Chinese translations while disabling remote translation downloads.
- Disabled launcher self-update checks.
- Replaced the application icon with Suika branding.
- Updated Unified Pass skin lookup to parse the sessionserver-compatible textures payload.
- Added `nide8auth.jar` as a separately-noticed third-party runtime component.
- Rebranded build/package metadata to Suika Launcher, while showing `西瓜幻想乡` in player-facing UI.
- Updated website/help/news links to `https://www.suika.fun/`.
- Disabled Microsoft login, offline login, CurseForge integration, global API-key settings and Imgur API-key overrides for this server build.
- Removed bundled Prism third-party API keys from default CMake configuration.

## Additional Runtime Component

`third_party/nide8auth/nide8auth.jar` is required at runtime for Unified Pass launches and is included as a separately-noticed third-party runtime component. It is not authored by Prism Launcher and is not part of Prism Launcher's GPL source code. See `NOTICE.md` and `third_party/nide8auth/README.md`.
