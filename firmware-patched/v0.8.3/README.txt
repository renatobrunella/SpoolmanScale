SpoolmanScale 0.8.3 + sdfp
==========================

Upstream SpoolmanScale v0.8.3 (Niko11111/SpoolmanScale) with the same additions
for 3dfilamentprofiles.com (sdfp) as the 0.8.2 build, carried over to 0.8.3.
Without the settings below the scale behaves like upstream.

Files
  firmware.bin          upload in the scale's web UI: Firmware page (OTA)
  firmware.factory.bin  full image for a USB/web flasher (bootloader+partitions)
  sdfp.patch            all changes against upstream v0.8.3 (patch -p1 in the
                        repo root; touches src/ and tools/lang_fr.jsonl)
  sounds/               the scan sounds as .wav, to listen to before flashing

What changed against the 0.8.2 build
  - Rebased on upstream 0.8.3 (new label editor, NIIMBOT printers, tag
    release on archive, unknown-spool card, spool by ID, ... - see the
    upstream release notes).
  - Labels: upstream now has its own filament label and a print sheet. With a
    label service set, More info still shows "Spool label" / "Filament label"
    and prints the service's label straight away; if the service gives none,
    BOTH now fall back to the scale's own layout (0.8.2 had no own filament
    label, so that one failed). Without a label service, More info is
    upstream's (print button, print sheet, FilaMan PC print), untouched.
    The browser label editor and the print sheet always print the scale's own
    layout - they print what they preview.
  - The label request now sends the printer head's dpi (dpi=203.2 for the
    Phomemo heads, as before; 300 for a 300 dpi NIIMBOT). The sdfp service
    already understands it, no service update needed.
  - Upstream 0.8.3 opens a card for an unknown spool. A tag that is about to
    link itself by its spool URL does not get that card.
  - The link-list fix (3. below) is still needed: 0.8.3 still ignores
    Spoolman's native tags there.

1. LABELS
   Scale web UI -> Printer page -> "Label service": http://<nuc-ip>:8710
   More info then offers "Spool label" and "Filament label", rendered by the
   sdfp service (it needs SPOOLMAN_URL and resvg-py). Either falls back to the
   scale's own label (the layout from the label editor) if the service is
   unreachable or answers with the wrong size.

2. SPOOL URL ON TAGS
   Writing: two extra tag formats, on the scale (Tag writing menu) and in the
   web UI (Tags page):
     Spool URL              https://spooldb.com/s/<code> as an NDEF URI record -
                            exactly what the phone app writes; fits an NTAG213
     Spool URL + OpenSpool  the URL, then the usual OpenSpool record, in one
                            NDEF message (needs an NTAG215)
   The code is the spool's sdfp short code (Spoolman extra sdfp_short_code, set
   by `sdfp spoolman all`); a spool without one cannot be written this way.

   A tag that already carries a spool URL keeps it: with OpenSpool or FilaMan
   selected, every write from the scale writes "Spool URL + OpenSpool" to it
   instead. On an NTAG213, where both do not fit, nothing is written and the
   URL stays.

   Reading: a tag that carries a spool URL and is not linked yet links itself to
   that spool on the first scan - no picking from the list. It goes through the
   scale's normal link, only when the lookup found nothing, and never takes a
   tag another spool holds. After linking itself, such a tag gets the same tag
   write a link from the list gets (Tag writing: Off / Ask / Always). A known
   spool whose tag carries only the URL is offered the OpenSpool record once
   (Ask) or gets it straight away (Always); Off leaves it alone.
   Archiving a spool with "release tags" (new in 0.8.3) erases a URL tag like
   any other NTAG.

3. FIX: LINKED SPOOLS IN THE LINK LIST (upstream bug, still in 0.8.3)
   Against Spoolman 0.27 the scale links into Spoolman's own tag relation. The
   link list only looked at the extra tag fields, so spools linked that way
   were still offered as free. They now count as linked.

4. SCAN SOUNDS
   Needs an 8 ohm speaker (1-2 W) on the board's SPK connector (onboard NS4168
   I2S amplifier). Setting: Backend -> More options -> "Scan sound" (tap to
   step Off / Low / Medium / High; a dropdown on the web backend page).
     known spool found     one short high beep        sounds/scan-found.wav
     unknown tag           two low beeps              sounds/scan-unknown.wav
     tag linked            a rising two-tone chime    sounds/scan-linked.wav

SIZE / PARTITIONS
   This build is 3,681,744 bytes (upstream 0.8.3 alone is about 3.65 MB), so
   it needs the 6 MB app slots, like upstream 0.8.3 itself. If the scale
   already runs one of the earlier sdfp builds over 3 MB, or upstream 0.8.3,
   it has them: upload firmware.bin on the Firmware page. If not, the Firmware
   page refuses before writing anything (safe to try); then flash
   firmware.factory.bin once over USB with the web flasher - settings stay.

Do NOT use "Update via GitHub" on the scale: it installs the upstream release
and drops these changes. Upstream releases need the patch re-applied and rebuilt.

License: upstream is GPL-3.0-or-later since 0.8.3. Sharing this build is
allowed under the GPL, as long as the source goes with it (upstream source +
sdfp.patch).
