#include "services/spool_url.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctype.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

#include "app/app_state.h"
#include "hardware/sd_logger.h"
#include "services/backend.h"
#include "services/spoolman_api.h"
#include "services/tag_link.h"
#include "services/tag_write.h"

#define SPOOL_URL_TIMEOUT_MS  5000

// Defined locally in every .cpp that needs it, as everywhere else in this
// project.
namespace {
struct SpiRamAllocator : ArduinoJson::Allocator {
  void* allocate(size_t size) override {
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!ptr) ptr = malloc(size);
    return ptr;
  }
  void deallocate(void* pointer) override { heap_caps_free(pointer); }
  void* reallocate(void* ptr, size_t new_size) override {
    void* p = heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    if (!p) p = realloc(ptr, new_size);
    return p;
  }
};
}

static bool startsWith(const char* s, const char* prefix) {
  return strncasecmp(s, prefix, strlen(prefix)) == 0;
}

bool spoolUrlShortCode(const char* url, char* code, size_t code_len) {
  if (!code || !code_len) return false;
  code[0] = 0;
  const char* p = url ? url : "";
  if (startsWith(p, "https://")) p += 8;
  else if (startsWith(p, "http://")) p += 7;
  if (startsWith(p, "www.")) p += 4;

  // The paths a spool's QR code or tag can carry: the detail page (/s/), the
  // edit page (/se/, the site's label default), and the long forms.
  const char* rest = nullptr;
  if (startsWith(p, "spooldb.com/s/"))       rest = p + 14;
  else if (startsWith(p, "spooldb.com/se/")) rest = p + 15;
  else if (startsWith(p, "3dfilamentprofiles.com/my/spool/")) rest = p + 32;
  if (!rest) return false;

  size_t n = 0;
  while (rest[n] && (isalnum((unsigned char)rest[n]) || rest[n] == '-' || rest[n] == '_')) n++;
  // What may follow the code: nothing, a slash, /edit, a query, a fragment.
  const char* tail = rest + n;
  if (*tail && *tail != '/' && *tail != '?' && *tail != '#') return false;
  if (!n || n >= code_len) return false;
  memcpy(code, rest, n);
  code[n] = 0;
  return true;
}

int spoolUrlFindSpool(const char* code) {
  if (!code || !code[0]) return 0;
  // Only the two fields compared, so even a server without the filter - which
  // then answers with the whole inventory - stays a small document.
  JsonDocument filter;
  filter[0]["id"] = true;
  filter[0]["extra"]["sdfp_short_code"] = true;
  SpiRamAllocator alloc;
  JsonDocument doc(&alloc);
  const int http = spoolmanFindSpoolByExtraField(backendBaseUrl(), "sdfp_short_code", code,
                                                 doc, SPOOL_URL_TIMEOUT_MS, &filter, nullptr);
  if (http != 200) return -1;
  // The filter is a partial, case insensitive match on the server, and short
  // codes are case sensitive: "abc" must not find "ABCd". Compare exactly.
  char want[32];
  snprintf(want, sizeof(want), "\"%s\"", code);  // extra values are JSON encoded
  for (JsonObjectConst sp : doc.as<JsonArrayConst>()) {
    const char* v = sp["extra"]["sdfp_short_code"] | "";
    if (!strcmp(v, want) || !strcmp(v, code)) return sp["id"] | 0;
  }
  return 0;
}

void spoolUrlAfterLookup() {
  if (backendMode() != BACKEND_SPOOLMAN) return;
  const TagInfo* ti = tagCachedInfo();
  if (!ti || !ti->spool_code[0]) return;
  if (sm_found && sm_id > 0) return;  // known already: nothing to do, no request
  const char* uid = tagCachedUid();
  if (!uid || !uid[0]) return;

  const int id = spoolUrlFindSpool(ti->spool_code);
  if (id < 0) {
    logSDf("SpoolURL: tag %s names spool code %s, Spoolman did not answer", uid,
           ti->spool_code);
    return;
  }
  if (id == 0) {
    logSDf("SpoolURL: tag %s names spool code %s, no Spoolman spool has it", uid,
           ti->spool_code);
    return;
  }
  if (tagLinkRequest(id, uid)) {
    logSDf("SpoolURL: tag %s names spool code %s -> linking to spool #%d", uid,
           ti->spool_code, id);
  } else {
    logSDf("SpoolURL: tag %s -> spool #%d, but a link is already waiting", uid, id);
  }
}
