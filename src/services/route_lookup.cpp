#include "services/route_lookup.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <cstring>

#include "services/adsb_client.h"

namespace services::route {

namespace {

constexpr char kApiBase[] = "https://api.adsbdb.com/v0/callsign/";
/** Be a good citizen towards a free, keyless public API. */
constexpr unsigned long kMinRequestGapMs = 2000;
constexpr unsigned long kRequestTimeoutMs = 4000;

struct RouteEntry {
  char callsign[9] = "";
  char orig[kAirportCodeLen] = "";
  char dest[kAirportCodeLen] = "";
  bool valid = false;  // a route was actually found for this callsign
};

// One entry per distinct callsign seen this session; wraps around (oldest
// evicted first) rather than growing unbounded — plenty for a live view
// that only ever shows kMaxAircraft planes at once.
constexpr size_t kCacheSize = services::adsb::kMaxAircraft;
RouteEntry s_cache[kCacheSize];
size_t s_cache_count = 0;
size_t s_cache_next = 0;
unsigned long s_last_request_ms = 0;

RouteEntry* findCached(const char* callsign) {
  for (size_t i = 0; i < s_cache_count; ++i) {
    if (strcmp(s_cache[i].callsign, callsign) == 0) {
      return &s_cache[i];
    }
  }
  return nullptr;
}

RouteEntry* reserveCacheSlot(const char* callsign) {
  RouteEntry* e = &s_cache[s_cache_next];
  *e = RouteEntry{};
  strncpy(e->callsign, callsign, sizeof(e->callsign) - 1);
  s_cache_next = (s_cache_next + 1) % kCacheSize;
  if (s_cache_count < kCacheSize) {
    ++s_cache_count;
  }
  return e;
}

void copyCode(JsonObject airport, char* out) {
  out[0] = '\0';
  if (airport.isNull() || !airport["iata_code"].is<const char*>()) {
    return;
  }
  strncpy(out, airport["iata_code"].as<const char*>(), kAirportCodeLen - 1);
  out[kAirportCodeLen - 1] = '\0';
}

/** Fetch + parse one callsign's route. entry->valid stays false (a normal,
 *  common outcome — most traffic has no published route) on any failure;
 *  either way the callsign is now cached so we never retry it. */
void performLookup(RouteEntry* entry) {
  String url = kApiBase;
  url += entry->callsign;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) {
    return;
  }
  http.setConnectTimeout(kRequestTimeoutMs);
  http.setTimeout(kRequestTimeoutMs);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    return;
  }

  JsonObject route = doc["response"]["flightroute"];
  if (route.isNull()) {
    return;
  }
  copyCode(route["origin"], entry->orig);
  copyCode(route["destination"], entry->dest);
  entry->valid = entry->orig[0] != '\0' && entry->dest[0] != '\0';
  if (entry->valid) {
    Serial.printf("route: %s  %s > %s\n", entry->callsign, entry->orig,
                  entry->dest);
  }
}

}  // namespace

bool lookup(const char* callsign, char* orig_out, char* dest_out) {
  orig_out[0] = '\0';
  dest_out[0] = '\0';
  if (callsign == nullptr || callsign[0] == '\0') {
    return false;
  }
  const RouteEntry* e = findCached(callsign);
  if (e == nullptr || !e->valid) {
    return false;
  }
  strncpy(orig_out, e->orig, kAirportCodeLen);
  strncpy(dest_out, e->dest, kAirportCodeLen);
  return true;
}

void poll() {
  const unsigned long now = millis();
  if (now - s_last_request_ms < kMinRequestGapMs) {
    return;
  }

  const size_t n = services::adsb::aircraftCount();
  const services::adsb::Aircraft* planes = services::adsb::aircraftList();
  for (size_t i = 0; i < n; ++i) {
    const char* callsign = planes[i].callsign;
    if (callsign[0] == '\0' || findCached(callsign) != nullptr) {
      continue;
    }
    s_last_request_ms = now;
    performLookup(reserveCacheSlot(callsign));
    return;  // at most one new lookup per call
  }
}

}  // namespace services::route
