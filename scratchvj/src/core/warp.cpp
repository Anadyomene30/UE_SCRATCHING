#include "core/warp.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kEpsilon = 1e-12;

bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

// Shortest distance from a point to a segment, used for the mask's feather.
double distance_to_segment(const Point& p, const Point& a, const Point& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length_squared = dx * dx + dy * dy;
    if (length_squared < kEpsilon) {
        return std::hypot(p.x - a.x, p.y - a.y);
    }
    double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / length_squared;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

}  // namespace

bool CornerPin::is_identity() const {
    return near(top_left.x, 0.0) && near(top_left.y, 0.0) && near(top_right.x, 1.0) &&
           near(top_right.y, 0.0) && near(bottom_right.x, 1.0) && near(bottom_right.y, 1.0) &&
           near(bottom_left.x, 0.0) && near(bottom_left.y, 1.0);
}

Homography homography_from(const CornerPin& pin) {
    // Heckbert's closed form for the unit square to an arbitrary quadrilateral.
    const double x0 = pin.top_left.x, y0 = pin.top_left.y;
    const double x1 = pin.top_right.x, y1 = pin.top_right.y;
    const double x2 = pin.bottom_right.x, y2 = pin.bottom_right.y;
    const double x3 = pin.bottom_left.x, y3 = pin.bottom_left.y;

    const double dx1 = x1 - x2, dx2 = x3 - x2, dx3 = x0 - x1 + x2 - x3;
    const double dy1 = y1 - y2, dy2 = y3 - y2, dy3 = y0 - y1 + y2 - y3;

    Homography h;
    double a, b, c, d, e, f, g, i;

    if (std::fabs(dx3) < kEpsilon && std::fabs(dy3) < kEpsilon) {
        // A parallelogram: the projective terms vanish and it is purely affine.
        a = x1 - x0; b = x2 - x1; c = x0;
        d = y1 - y0; e = y2 - y1; f = y0;
        g = 0.0; i = 0.0;
    } else {
        const double denominator = dx1 * dy2 - dx2 * dy1;
        if (std::fabs(denominator) < kEpsilon) {
            // Collinear corners: there is no such projection. Refuse rather than
            // hand back a matrix of infinities that would paint nothing at all.
            return Homography{};
        }
        g = (dx3 * dy2 - dx2 * dy3) / denominator;
        i = (dx1 * dy3 - dx3 * dy1) / denominator;
        a = x1 - x0 + g * x1;
        b = x3 - x0 + i * x3;
        c = x0;
        d = y1 - y0 + g * y1;
        e = y3 - y0 + i * y3;
        f = y0;
    }

    h.m[0] = a; h.m[1] = b; h.m[2] = c;
    h.m[3] = d; h.m[4] = e; h.m[5] = f;
    h.m[6] = g; h.m[7] = i; h.m[8] = 1.0;
    return h;
}

Point apply(const Homography& h, const Point& p) {
    const double w = h.m[6] * p.x + h.m[7] * p.y + h.m[8];
    if (std::fabs(w) < kEpsilon) return Point{0.0, 0.0};
    return Point{(h.m[0] * p.x + h.m[1] * p.y + h.m[2]) / w,
                 (h.m[3] * p.x + h.m[4] * p.y + h.m[5]) / w};
}

Homography invert(const Homography& h, bool& ok) {
    const double* m = h.m;
    const double c00 = m[4] * m[8] - m[5] * m[7];
    const double c01 = m[5] * m[6] - m[3] * m[8];
    const double c02 = m[3] * m[7] - m[4] * m[6];
    const double determinant = m[0] * c00 + m[1] * c01 + m[2] * c02;

    if (std::fabs(determinant) < kEpsilon) {
        ok = false;
        return Homography{};
    }
    ok = true;

    Homography out;
    out.m[0] = c00 / determinant;
    out.m[1] = (m[2] * m[7] - m[1] * m[8]) / determinant;
    out.m[2] = (m[1] * m[5] - m[2] * m[4]) / determinant;
    out.m[3] = c01 / determinant;
    out.m[4] = (m[0] * m[8] - m[2] * m[6]) / determinant;
    out.m[5] = (m[2] * m[3] - m[0] * m[5]) / determinant;
    out.m[6] = c02 / determinant;
    out.m[7] = (m[1] * m[6] - m[0] * m[7]) / determinant;
    out.m[8] = (m[0] * m[4] - m[1] * m[3]) / determinant;
    return out;
}

void Mask::set_polygon(std::vector<Point> points) { points_ = std::move(points); }

double Mask::coverage(const Point& p) const {
    if (empty()) return 1.0;  // no mask set means nothing is masked

    // Crossing test, which handles concave outlines as well as convex ones.
    bool inside = false;
    for (std::size_t i = 0, j = points_.size() - 1; i < points_.size(); j = i++) {
        const Point& a = points_[i];
        const Point& b = points_[j];
        if ((a.y > p.y) != (b.y > p.y)) {
            const double x = (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
            if (p.x < x) inside = !inside;
        }
    }

    if (feather_ <= kEpsilon) return inside ? 1.0 : 0.0;

    double distance = 1e30;
    for (std::size_t i = 0, j = points_.size() - 1; i < points_.size(); j = i++) {
        distance = std::min(distance, distance_to_segment(p, points_[i], points_[j]));
    }

    // Ramp across the border rather than only inwards, so the softened edge is
    // centred on the outline the user actually dragged.
    const double half = feather_ * 0.5;
    const double signed_distance = inside ? distance : -distance;
    return std::clamp((signed_distance + half) / feather_, 0.0, 1.0);
}

}  // namespace svj
