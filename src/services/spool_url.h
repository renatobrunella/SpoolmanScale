#pragma once
// ============================================================
//  spool_url.h - tags that carry the spool's page URL
//  sdfp fork addition (SPOOLMANSCALE-SDFP.md, section 4, option C)
// ============================================================
//
// 3dfilamentprofiles.com gives every spool a short code and a page,
// https://spooldb.com/s/<code>. A tag with that URL as an NDEF URI record -
// written by a phone, or by the scale's "Spool URL" formats - names its spool
// by itself. The sdfp import puts the same code on the Spoolman spool, as the
// extra field sdfp_short_code.
//
// So a tag the scale does not know yet, but whose URL names a spool, is linked
// to that spool on the spot instead of waiting for someone to pick it from the
// list. Only then: a tag that is already linked costs no request, and a tag
// another spool holds is never taken from it (tagLinkRequest() refuses).

#include <stddef.h>

// The short code in a spool URL - spooldb.com/s/<code>, /se/<code>, or
// 3dfilamentprofiles.com/my/spool/<code>[/edit], with or without scheme or www.
// False, and code empty, for anything else (a filament or location page too).
bool spoolUrlShortCode(const char* url, char* code, size_t code_len);

// The Spoolman spool carrying this short code in extra.sdfp_short_code, by an
// exact comparison. 0 when there is none, -1 when the server did not answer.
// Blocking: from appLoop() only.
int spoolUrlFindSpool(const char* code);

// From appLoop(), right after the lookup of a freshly scanned NTAG. If the tag
// carries a spool URL and the lookup found no spool for it, parks a link of the
// tag to the spool the URL names (tagLinkRequest, which waits for a lookup still
// running and refuses a tag held elsewhere). Does nothing outside Spoolman mode.
void spoolUrlAfterLookup();
