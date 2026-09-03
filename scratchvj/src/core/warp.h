// scratchvj — output geometry: corner pin and mask.
//
// Projecting onto a surface you are not square to is the ordinary case, not the
// exception, so the two things that fix it belong in the application: dragging
// the four corners, and masking off what should not spill.
//
// Corner pin is a HOMOGRAPHY, not a bilinear stretch. The difference is visible
// and it is the whole reason to do it properly: under a real projector the centre
// of the image does NOT land at the centre of the quad, because perspective
// compresses the far side. A bilinear warp puts it at the centroid and everything
// inside the picture shears. In a shader this is one 3x3 matrix.
//
// Mesh warp and edge blending across projectors are deliberately NOT here: those
// are a project of their own, and the output already goes out over Spout and NDI
// to tools that do them well.
#pragma once

#include <vector>

namespace svj {

struct Point {
    double x = 0.0;
    double y = 0.0;
};

// Row-major 3x3, with m[8] normalised to 1 where possible.
struct Homography {
    double m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
};

// The four corners the unit square is pinned to, in order: top-left, top-right,
// bottom-right, bottom-left.
struct CornerPin {
    Point top_left{0.0, 0.0};
    Point top_right{1.0, 0.0};
    Point bottom_right{1.0, 1.0};
    Point bottom_left{0.0, 1.0};

    bool is_identity() const;
};

// The transform taking the unit square to the pinned quad. A degenerate quad
// yields the identity rather than a matrix full of infinities.
Homography homography_from(const CornerPin& pin);

Point apply(const Homography& h, const Point& p);

// The inverse transform. `ok` is false when the matrix cannot be inverted.
Homography invert(const Homography& h, bool& ok);

// A mask polygon with a soft edge. Returns how much of the output survives at a
// point: 1 inside, 0 outside, and a feathered ramp across the border.
class Mask {
public:
    void set_polygon(std::vector<Point> points);
    void set_feather(double feather) { feather_ = feather; }

    bool empty() const { return points_.size() < 3; }
    const std::vector<Point>& points() const { return points_; }

    double coverage(const Point& p) const;

private:
    std::vector<Point> points_;
    double feather_ = 0.0;
};

}  // namespace svj
