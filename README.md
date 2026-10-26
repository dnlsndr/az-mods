# Record Streaming

Keeps **MASTER REC** running while a track from a streaming service plays.

The deck normally disables recording when a streaming service is the source, and it stops an
active recording when a streaming track is loaded. This mod removes that one block. All the
other recording checks stay in place: a recording device must still be selected, Bluetooth
input is still refused, and a track that is still loading still holds recording until it is
ready.

## Install from the AZ-MODS app

Add the repository `dnlsndr/az-mods-record` under **Community Mods > Repositories**, then open
**Browse > Record Streaming > Install**. The deck asks once to allow the build; approve it and
press **Reload now**.

## Supported build

- EP147 2.00, firmware tag `ep147-9e9454e357389ece`.
- AZ-MODS core 1.2.3 or newer, mod API 1.
- The mod refuses a different tag, or a build whose first 16 bytes at either hooked address
  differ from the tested build. Do not bypass those guards.

## What it changes

The recorder asks `usecase::usb_rec::UsbRecController` whether recording may start. That
check returns 1 for yes and 0 for no, and notifies the reason through a second function. The
streaming reason is 6, set when a loaded track's `LoadTrackInfo` source is a streaming source.

| Address | Role | Hook |
|---|---|---|
| `0xd5cb78` | the startable check | calls the original; turns a 0 with reason 6 into 1 while the row is on |
| `0xd65990` | the reason notification | records the reason only while the check is running |

Both are function-entry hooks installed with `claim_hook`. No code bytes are patched. Every
other reason passes through untouched, and with the Mod Menu row off the mod behaves exactly
like stock.

## Install by hand (USB)

1. Build the library (below).
2. Plug the AZ-MODS USB into the computer, close the app if it is open, and run
   `./test/install-usb.sh "/Volumes/<USB>"`. This copies the library, `azmod.json` and the
   icon to `MODS/community-mods/record-streaming/` and adds
   `community-mods.record-streaming = on` to `MODS/azmods.cfg`.
3. Open AZ-MODS, select the USB, open **Community Mods > Installed**. **Record Streaming**
   is listed; its switch must be on. If the page says all community mods are off, use
   **Turn on** above the list.
4. Eject the USB, boot the deck with it, open **Mod Menu > Community Mods > Installed**,
   approve **Record Streaming**, then **Reload now**.
5. The row **Recording while streaming** appears on the mod's page. It is on by default.

The app has no "install from a file" button: **Browse** installs from a repository. To make
it appear there, publish a repository with `azmods-repo.json` and the `.azmod` package from
`dist/`, then add the repository under **Community Mods > Repositories**. Copying the folder
and using the **Installed** switch is the same install without that step.

## Install on the deck only

If the USB already has the files, the deck can do the rest:

1. Add `community-mods.record-streaming = on` to `MODS/azmods.cfg`.
2. Power-cycle with the USB in, open **Mod Menu > Community Mods > Installed**, approve
   **Record Streaming**, then **Reload now**.
3. The row **Recording while streaming** appears on the mod's page.

Over SSH, the deck loads mods from `/home/root/settings/az-usbload/community/`. Swap the
library there, add its SHA-256 to `/home/root/settings/azmod-community/approved`, restart
`EP147.service`.

## Build

```sh
<path-to-az-mods-sdk>/tools/azmod-pack build .
```

or, inside the SDK's arm64 Ubuntu 18.04 image:

```sh
make AZ_SDK_INCLUDE=/sdk/include
```

## Test

```sh
make test
```

compiles the two hooks with stand-in EP147 functions and checks the decision table: reason 6
is overridden only while the row is on, and reasons 1, 3, 4 and 5 are never changed.

On a deck, test in this order:

1. Local track, select a USB recording device, press REC. It must start and produce a WAV.
2. Stop recording.
3. Load a streaming track, press REC. It must start.
4. While recording, load another streaming track. Recording must continue.
5. Turn the row off. With a streaming track loaded, REC must be refused again.

A streaming account is needed for steps 3 to 5.

With logs on (`touch /home/root/settings/azmod-logs.on`), the mod writes
`/tmp/azmod-community/record-streaming.log`. A refused start records
`recording refused, reason N`; `N` should be 6 while a streaming track is loaded. Any other
number means this build's reason table differs; turn the row off and report the number with
the firmware tag.

## Limits

- It only knows EP147 2.00. A firmware update changes the tag and the code, and the mod stays
  off until the addresses and fingerprints are checked on the new build.
- It changes the deck's recording policy. Recording a streaming service may conflict with
  that service's terms. Use it only for content you are allowed to record.
- A fault in the mod stops EP147 like a fault in any mod. The core switches this mod off at
  the next start, but a crash still interrupts the music.

## License

GPL-3.0-only. See `LICENSE`.
