# Spotify Highest Quality

Raises the Spotify DJ SDK's quality argument to the highest level the deck's
integration exposes.

EP147 calls the Spotify DJ SDK's `DJDownload_add_track` with a `DJAudioQuality`
argument. This mod hooks that function on EP147 2.00 and raises a lower value to
`8`, the SDK's own "8 (Highest quality)" entry. Values already at 8, or above it,
are left alone. The original function gets every other argument unchanged, and its
return value is passed back.

## Not lossless

This is the highest lossy level the deck's Spotify integration has. It is not
Spotify Lossless/FLAC. The Spotify hardware eSDK exposes `Hifi24` only as track
metadata; its playback bitrate API has no lossless setting.

The mod logs `DJAudioQuality <old> -> 8` for each track it changes, under
`/tmp/azmod-community/spotify-quality.log`.

## Supported build

- EP147 2.00, firmware tag `ep147-9e9454e357389ece`.
- AZ-MODS core 1.2.3 or newer, mod API 1.
- The mod verifies the first 16 bytes of `DJDownload_add_track` at `0x2827914`
  before claiming the hook, and refuses any other build.

## Install

Add the repository `dnlsndr/az-mods` under **Community Mods > Repositories**, then
**Browse > Spotify Highest Quality > Install**. Approve it on the deck and press
**Reload now**.

The Mod Menu row is **Highest Spotify quality**, on by default. Turn it off to
leave the SDK call untouched.

## Test

```sh
make test
```

The host test checks that the quality is raised only when the row is on and the
incoming value is below 8, and that an already-high value, the off state and the
return value pass through unchanged.

On a deck, play a Spotify track and read the log; it should say
`DJAudioQuality <old> -> 8`. The audible effect is the SDK's highest lossy stream,
not FLAC.

## License

GPL-3.0-only. See `LICENSE`.
