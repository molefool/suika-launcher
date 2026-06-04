# Notices

Suika Launcher is a modified fork of Prism Launcher.

This project is not Prism Launcher and is not endorsed by or affiliated with the Prism Launcher project.

## Upstream Works

- Prism Launcher contributors retain copyright in Prism Launcher portions.
- PolyMC contributors retain copyright in PolyMC portions inherited by Prism Launcher.
- MultiMC contributors retain copyright in MultiMC portions inherited by Prism Launcher.

The original license files and attribution files are retained:

- `LICENSE`
- `COPYING.md`
- `program_info/LICENSE`
- `program_info/README.md`

## Suika Launcher Modifications

Suika Launcher modifications were added by Suika Launcher contributors beginning on 2026-06-04. Major modifications are listed in `FORK_NOTICE.md`, including the Suika server-only Unified Pass login flow, fixed server ID, simplified Chinese defaults and disabled third-party integrations.

## Unified Pass Runtime Agent

`third_party/nide8auth/nide8auth.jar` is an independent runtime component provided by nide8/Unified Pass for Unified Pass authentication.

- Implementation title: `nide8auth`
- Implementation vendor: `nide8`
- Implementation version: `2.8`
- SHA1: `ceac742d5a7b6eed02765eb8b0786729eea19c7c`

It is installed next to the launcher executable so Unified Pass launches can pass:

```text
-javaagent:nide8auth.jar=<serverId>
-Dnide8auth.client=true
```

The jar is not authored by the Prism Launcher project and is not part of Prism Launcher's GPL source code.
