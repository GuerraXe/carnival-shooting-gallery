#pragma once

namespace gallery {

// Game time in seconds. double so a 60 s round accumulated from thousands of
// small frame deltas does not drift (ARCHITECTURE §4.2).
using Seconds = double;

// Largest frame delta the simulation will ever see (D-007, ARCHITECTURE §5.2).
// Window drags / debugger pauses stall the loop; without the clamp one huge dt
// would teleport targets and burn the round timer.
inline constexpr Seconds kMaxFrameDelta = 0.100;

// Clamp a raw measured frame delta into [0, maxDelta].
// Negative or NaN input (should be impossible from a monotonic clock) yields 0.
[[nodiscard]] Seconds clampFrameDelta(Seconds raw, Seconds maxDelta = kMaxFrameDelta) noexcept;

} // namespace gallery
