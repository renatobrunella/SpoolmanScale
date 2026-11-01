# Porting the sdfp changes from SpoolmanScale 0.8.2 to 0.8.3

Date: 2026-11-01. Result: `SpoolmanScale-0.8.3/` patched in place, and this folder
(`firmware.bin`, `firmware.factory.bin`, `sdfp.patch`, `README.txt`, `sounds/`).
Status: builds clean, upstream checks and tests pass. **Not yet flashed.**

## What was carried over

All four sdfp features from `dist/spoolmanscale-0.8.2-sdfp/sdfp.patch`:

1. **Labels from the sdfp label service.** Setting `label_svc`, More info shows the buttons
   "Spool label" and "Filament label".
2. **Spool URL on tags.** Two tag formats, Spool URL and Spool URL + OpenSpool. A tag with a
   spool URL links itself to its spool, and a URL already on a tag is kept.
3. **Link-list fix.** Spools linked through Spoolman 0.27's native tags count as linked.
4. **Scan sounds.** Played through the NS4168 speaker; the setting is `sound_vol`.

## Method

The goal was a real three-way merge, with conflict markers, instead of `patch` dropping
`.rej` files. All of this ran in `/tmp/ss083/`, so the original folders were never touched.

1. **Rebuild pristine 0.8.2.** Copy `SpoolmanScale-0.8.2/`, which is patched in place, and
   reverse the old patch:
   `patch -p1 -R < sdfp.patch`.
2. **Set up a git repo in `/tmp/ss083/git`:**
   - commit `upstream 0.8.2` (the pristine tree);
   - branch `sdfp`, then apply the 0.8.2 patch and commit. Diffing this commit against the
     user's patched 0.8.2 folder showed them identical, which proves the starting point is
     right.
   - On branch `up`, commit `upstream 0.8.3` (a copy of `SpoolmanScale-0.8.3/`).
3. **Rebase:** run `git rebase up` on branch `sdfp`, with `merge.conflictstyle zdiff3`.
4. **Resolve the conflicts** (see below) and adapt the code to 0.8.3's API changes.
5. **Build, check and test.**
6. **Generate the patch** with `diff -ruN a b`, where `a` is pristine 0.8.3 and `b` is
   `git archive HEAD`.
   - Verify: apply it to a fresh copy of 0.8.3 and diff against `b`. They were identical.
   - Then apply it to the user's `SpoolmanScale-0.8.3/`, which was confirmed pristine first.
     The result also matched `b`.

As a first quick test, a dry run of the old patch against 0.8.3 failed hunks in 13 files.
Under the git rebase, 12 files had real conflicts. The rest merged automatically.

## Conflicts and how they were resolved

| File | Conflict | Resolution |
|---|---|---|
| `app/app_loop.cpp`, `ui/lookup_scan.cpp` | includes next to the new `dried_migrate.h` | keep both |
| `app/deferred_actions.{h,cpp}` | upstream added `print_last_filament_label_pending` and others | keep both; ours stays `print_filament_label_pending` |
| `services/label_printer.{h,cpp}` | new result codes (NIIMBOT, `LP_SERVER_LABEL`) | keep upstream's; **dropped** our `LP_LABEL_SERVICE` (see labels) |
| `lang.cpp` | new rows near `STR_W_C_TAGOPTS`, and upstream's French rewording of that row | our rows plus upstream's TAGOPTS row. The enum in `lang.h` and the table were checked to align (1475 = 1475) |
| `ui/tag_write_popup.cpp` | upstream moved the queueing into `queueTagJob()` and added `tagWriteRequestErase()` | upstream's version, with `tagWriteFormatFor(g_tagwrite_fmt)` applied inside `queueTagJob()` |
| `ui/printer_screen.cpp` | upstream rewrote printing (`printSpoolLabel` / `spoolRaster`, label kinds, server labels) | redone on top of upstream; see labels |
| `ui/more_info_screen.cpp` | upstream's print button gained the print sheet and FilaMan PC print | upstream's block kept; ours placed before it as an `if` branch |
| `web/pages/page_printer.cpp` | page split into functions, new state JSON fields | our card became `labelServiceCard()`, called after the printer card; `labelsvc` added to the JSON and to `paint()` |
| `tools/lang_fr.jsonl` | rows around ours | see the French section |

The new files (`label_fetch`, `label_buttons`, `sound`, `spool_url`) showed as modify/delete
conflicts, which was a rename-detection artefact. They were taken as they were.

## Adaptations to 0.8.3 (beyond the conflicts)

### Labels
0.8.3 has its **own filament label** (`labelRenderFilament`), label kinds
(`LABEL_KIND_SPOOL` / `LABEL_KIND_FILAMENT`), a print sheet with a preview, a browser
label editor, and FilaMan server presets (`labelServer*`).

- **Service lookup.** `printSpoolLabel()` gained a `service` parameter. When it is set,
  `labelFetchRaster()` runs first. If that fails, the label falls back to `spoolRaster()`,
  which is upstream's own layout. That fallback now covers **both** label kinds, so
  `LP_LABEL_SERVICE` and `STR_PRN_ERR_LABEL_SVC` were removed as unused.
- **Which paths use the service.** Only `printerPrintPadLabel()` does, and only the More info
  buttons call it. The print sheet and the browser editor print the scale's own layout,
  because that is what they preview.
- **More info header.** If `bt_print`, a known spool and `labelServiceConfigured()` all
  hold, `addLabelPrintButtons()` shows two buttons that print directly. Otherwise upstream's
  block runs unchanged.
- **`label_buttons.cpp`.** Simplified so it always draws the two buttons; upstream owns the
  single-button case.
- **Per-model printer API.** `labelPrinterDotsForMm(mm)` became
  `labelPrinterDotsForMm(model, mm)`.
  - The size check now uses `labelPrinterContentWidth(model, w)` and
    `labelPrinterDotsForMm(model, h)`.
  - The request sends `&dpi=`: `203.2` for 203 dpi heads (8 dots/mm, as before) and `300`
    for 300 dpi NIIMBOTs.
  - `tools/sdfp_service.py` already accepted `dpi`, so it needs no change.
- **Parameter limit.** `labelFetchRaster()` dropped its `why` / `why_len` parameters and now
  logs the reason itself. This keeps upstream's conventions ratchet (at most 5 parameters)
  green.

### Spool URL vs. the new unknown-spool card
0.8.3 opens a card for a spool no inventory knows (`unknownSpoolOffer` in
`lookupFollowUp`). Before this change, a URL tag that was linking itself would flash that
card until the link landed.

- New: `spoolUrlLinkExpected(uid)`. It is true for 15 s after `spoolUrlAfterLookup()`
  parks a link for that tag, and false again once the link is taken.
- `lookupFollowUp()` skips the card in that case. The sounds already skipped the "unknown"
  beep in the same case.

### Checked and left as they were
- **Link-list bug.** It is still present upstream: 0.8.3's `spoolHasAnyTag()` still ignores
  `spool["tags"]`, so the fix is still needed.
- **New tag-write callers.** The new 0.8.3 flows (`tag_create*`, `spool_start`, tag
  companion) do not write tags. The only callers are the ones already covered:
  `tag_write_popup` and `page_tags`.
- **Tag release on archive.** Release erases NTAGs only if `tagCachedHasRecord()` is true,
  and a URL tag counts as having a record, so it gets erased like any other NTAG.
- **Archived spools.** `spoolUrlFindSpool` uses the standard spool search, which leaves out
  archived spools.

### French strings (`tools/lang_fr.jsonl`)
Upstream treats the jsonl as the source of the French column and generates `lang.cpp` from
it (`tools/lang_fr.py`).

- Our 15 rows had their French added with `apply`, taken from the French already in our
  `lang.cpp`. `coverage` then showed 0 missing.
- `seed` rewrites the source line numbers in every row, which bloated the patch to 600 KB.
  So the final jsonl is **upstream's file plus our 15 rows**, inserted in enum order.
- `emit` then produced a `lang.cpp` byte-identical to the built one.

## Build environment

The old `/tmp/fwbuild` was gone, so it was recreated. The user's `~/.platformio` was not
touched.

- **venv:** `/tmp/fwbuild/venv`, with `PLATFORMIO_CORE_DIR=/tmp/fwbuild/core`.
- **Python 3.14 is required.** Homebrew's default is now 3.15, which pioarduino 55.03.312-1
  rejects.
- **Corporate proxy TLS:** truststore has to be injected in two places.
  - **venv:** with a `.pth` file containing
    `import truststore; truststore.inject_into_ssl()`. A `sitecustomize.py` in the venv
    does **not** work, because Homebrew's global `sitecustomize` shadows it.
  - **PlatformIO's own `core/penv`**, which the build creates: copy the `truststore`
    package in, plus the same `.pth`.
- **Build command:**
  `pio run -e wt32-sc01-plus` in `/tmp/ss083/git`. The toolchain was Arduino core 3.3.12 on
  ESP-IDF 5.5.5.

## Verification

- **Build:** succeeded.
  - RAM 35.0%; flash 3,588,151 of 6,291,456 bytes (57%).
  - `firmware.bin` is 3,681,744 bytes.
  - No compiler warnings in any file we touch.
- **`scripts/check.sh`:** passed with warnings. The warnings are upstream's UTF-8 note in
  `spool_flow.cpp`, plus "files over 1000 lines grew" for our additions.
- **Upstream tests:** all 8 files in `test/*.py` pass.
- **Patch:** it applies to pristine 0.8.3 without fuzz or rejects, and the result is
  identical to the built source.
- **On the scale:** **not tested yet.** Things to try after flashing:
  - the More info label buttons (spool and filament, plus the fallback with the service
    stopped);
  - auto-link of a URL tag, with no unknown-spool card;
  - the URL + OpenSpool write;
  - the link list leaving out natively linked spools;
  - the sounds.

## Flashing notes

- The image is over 3 MB, so it needs the 6 MB app slots. Upstream 0.8.3 needs them as well.
- The Firmware page refuses an image that doesn't fit before writing anything. In that case,
  flash `firmware.factory.bin` once over USB; settings are kept.
- Never use "Update via GitHub", because it installs plain upstream.

## Next upstream release

Repeat the method above. It is quickest with the git repo: keep `/tmp/ss083/git`, or
rebuild it from this patch. Add the new release as a commit on `up` and run
`git rebase up` on `sdfp`. Then:

- For `tools/lang_fr.jsonl`, take upstream's file and re-insert only our rows.
- Run `scripts/check.sh` and `test/*.py`.
- Regenerate the patch with `diff -ruN` against the pristine release.
