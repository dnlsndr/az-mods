# Spotify Highest Quality

Adds a **Spotify quality** choice row to the Mod Menu: Deck default, Fastest (0),
Default (5) or Highest (8).

EP147 calls the Spotify DJ SDK's `DJDownload_add_track` with a `DJAudioQuality`
argument. This mod hooks that function on EP147 2.00 and replaces the argument with
the selected level. **Deck default** leaves EP147's own value alone. The original
function gets every other argument unchanged, and its return value is passed back.

The choices are the three levels the SDK names: 0 (Fastest), 5 (Default) and 8
(Highest). The intermediate values 1-4 and 6-7 are not named by its table and are
not exposed.

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

The row is **Spotify quality**, set to **Highest (8)** by default. Choose **Deck
default** to leave the SDK call untouched.

## Test

```sh
make test
```

The host test checks that the selected level replaces w2, that Deck default leaves
the incoming value alone, and that the return value passes through unchanged.

On a deck, play a Spotify track and read the log; it should say
`DJAudioQuality <old> -> 8`. The audible effect is the SDK's highest lossy stream,
not FLAC.

## License

GPL-3.0-only. See `LICENSE`.
