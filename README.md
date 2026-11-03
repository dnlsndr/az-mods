# AZ-MODS mods

Community Mods for the XDJ-AZ, made by dnlsndr.

Add this repository in AZ-MODS under **Community Mods > Repositories**:

```text
dnlsndr/az-mods
```

Then **Browse** shows every mod below, and **Install** puts it on the USB. The deck asks
once to allow each build; approve it and press **Reload now**.

## Mods

| Mod | What it does |
|---|---|
| [Record Streaming](record-streaming/) | Keeps MASTER REC running while a track from a streaming service plays. |

## Repository layout

```text
azmods-repo.json                 the list AZ-MODS reads: one entry per mod
record-streaming/                one folder per mod
  azmod.json                     id, version, firmware tag, hooks
  Makefile
  record-streaming.c
  icon.png
  LICENSE
  test/
.github/workflows/
  release-record-streaming.yml   build and publish this mod on its tag
```

Adding a mod means adding its folder, its workflow, and its entry in `azmods-repo.json`:

```sh
azmod-pack package record-streaming --github dnlsndr/az-mods --tag record-streaming-<version>
azmod-pack package next-mod        --github dnlsndr/az-mods --tag next-mod-<version>
azmod-pack repo add azmods-repo.json dist/record-streaming-<version>.entry.json
azmod-pack repo add azmods-repo.json dist/next-mod-<version>.entry.json
azmod-pack repo check azmods-repo.json
```

Every mod id is unique across all AZ-MODS repositories. Each mod has its own release tag,
`.azmod` package and icon; the repository file points at those release assets.

## Build one mod

```sh
<az-mods-sdk>/tools/azmod-pack build record-streaming
```

## License

GPL-3.0-only. Each mod folder has its own copy of the license.
