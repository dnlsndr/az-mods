# Spotify Highest Quality

Adds a **Spotify quality** choice row to the Mod Menu: **Deck default**, Fastest
(0) or Default (5).

EP147 calls the Spotify DJ SDK's `DJDownload_add_track` with a `DJAudioQuality`
argument. This mod hooks that function on EP147 2.00 and replaces the argument with
the selected level. **Deck default** leaves EP147's own value alone. The original
function gets every other argument unchanged, and its return value is passed back.

The level **8 ("Highest quality") is not offered**: loading a Spotify track with
it crashed EP147. The intermediate values 1-4 and 6-7 are not named by the
SDK's table and are not exposed. The safe levels are 0 (Fastest) and 5 (Default).

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

The row is **Spotify quality**, set to **Deck default** by default. The first
`add_track` call logs the deck's own value before any choice is applied:

```text
first add_track quality 5
```

Choose **Deck default** to leave the SDK call untouched.

## Test

```sh
make test
```

The host test checks that the selected level replaces w2, that Deck default leaves
the incoming value alone, and that the return value passes through unchanged.

On a deck, play one Spotify track and read the log. The first line is the deck's
own quality. If you select Fastest or Default, later tracks log
`DJAudioQuality <old> -> <new>`.

## License

GPL-3.0-only. See `LICENSE`.
