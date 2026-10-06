// Image correlation matcher for the Goodix 52xd (27c6:521d) press sensor

// Copyright (C) 2026 denizberkin <berkindeniz2000@gmail.com>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#pragma once

#include <glib.h>

// Correlation a probe must reach against one enrolled frame to match.
#define GOODIX52XD_MATCH_THRESHOLD 0.77

/**
 * goodix52xd_match_score:
 * @enrolled: @n_enrolled raw frames captured during enrollment
 * @n_enrolled: number of enrolled frames, at least 1
 * @probe: raw frame to verify
 * @width: frame width
 * @height: frame height
 * @stop_at: return as soon as a score of at least this is found
 *
 * All frames are averaged 12-bit sensor values, row-major. The sensor's fixed
 * pattern is estimated from the enrolled frames and removed from every frame,
 * then a rotated central patch of each frame is located in the other by
 * normalised cross-correlation.
 *
 * Returns: the best correlation between @probe and any enrolled frame.
 */
double goodix52xd_match_score(const float* const* enrolled, guint n_enrolled,
                              const float* probe, int width, int height,
                              double stop_at);
