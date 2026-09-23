#pragma once

#include <cstddef>

namespace services::route {

/** 3-letter IATA airport code + NUL. */
constexpr size_t kAirportCodeLen = 4;

/**
 * Cached route for a callsign, if one was found. Never makes a network
 * call itself — writes IATA codes (e.g. "AMS", "LHR") into orig_out/
 * dest_out and returns true only if poll() already looked this callsign up
 * and found a published route. Most GA/military/charter traffic has none
 * (not an error) — returns false and clears both buffers.
 */
bool lookup(const char* callsign, char* orig_out, char* dest_out);

/**
 * Looks up the route (via api.adsbdb.com, free/no key) for at most one
 * not-yet-cached callsign currently on screen, throttled to a couple of
 * seconds between requests. Call every loop() tick — cheap no-op when
 * there's nothing new to look up or the throttle hasn't elapsed.
 */
void poll();

}  // namespace services::route
