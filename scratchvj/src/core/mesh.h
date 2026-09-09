// scratchvj — the warp mesh: a grid of control points, straight or curved.
//
// core/warp's corner pin fixes the ordinary case, a flat surface seen off
// square, and it fixes it EXACTLY because a homography is exactly what
// perspective does. This file is for the case a homography cannot express: a
// curved screen, a corner of a room, a stretched sail. There the surface has to
// be pulled about by hand, and the tool for that is a grid whose points move.
//
// The roadmap said no to mesh warp, on the grounds that it is a project of its
// own and Spout already hands the picture to Resolume or MadMapper. That
// reasoning held while there was no output at all. Now that the program leaves
// the machine and the pin is already draggable, making it a grid is a day's
// work rather than a project, and one fewer piece of software between the
// instrument and a wall. The exclusion is withdrawn deliberately, not forgotten.
//
// Two ways to interpolate between the points:
//
//   Bilinear -- straight edges between neighbours. Predictable, and what you
//     want on flat panels meeting at hard angles.
//   Bezier   -- a Catmull-Rom surface through the same points, so the shape
//     curves smoothly and a single point moves its whole neighbourhood. What a
//     curved screen actually needs, and what Resolume's handles feel like.
//
// The grid always passes THROUGH its control points in both modes. That is not
// a detail: a point you drag must land where you dropped it, or the tool argues
// with your hand.
//
// Each grid line also carries its own SOURCE coordinate rather than being
// assumed evenly spaced. That is what lets a new line be inserted without the
// picture moving: the existing lines keep the piece of the source image they
// already had, and the new one takes the midpoint. Assume even spacing instead
// and every insertion redistributes the whole image -- which is exactly the
// lurch this tool must never produce while it is aimed at a wall.
#pragma once

#include <cstdint>
#include <vector>

#include "core/warp.h"

namespace svj {

enum class WarpInterpolation : std::uint8_t {
    Bilinear,
    Bezier,
};

class WarpMesh {
public:
    // A `cols` x `rows` grid evenly spread over the unit square. Two by two is
    // the corner pin's own shape, which is why that is the minimum.
    void reset(int cols = 2, int rows = 2);

    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // Control points, indexed from the top-left. Out-of-range access is a
    // caller bug and returns a corner rather than reading past the array.
    const Point& at(int col, int row) const;
    void set(int col, int row, Point value);

    void set_interpolation(WarpInterpolation mode) { mode_ = mode; }
    WarpInterpolation interpolation() const { return mode_; }

    // Where the source point (u, v), both in 0..1, lands on the output.
    Point map(double u, double v) const;

    // Inserts a column (or row) between the existing ones, at the midpoint of
    // the parameter span. New points are SAMPLED FROM THE CURRENT SURFACE, so
    // adding detail does not move the picture -- exactly in Bilinear, and to
    // within the curvature of the cell in Bezier, where a Catmull-Rom through
    // more points is genuinely a different curve. Adding a point mid-show must
    // not make the image lurch, which is why they are sampled rather than
    // interpolated linearly between neighbours.
    //
    // `before` is the index of the column the new one is inserted in front of,
    // clamped to 1..cols()-1. False when the grid is already at its limit.
    bool insert_column(int before);
    bool insert_row(int before);

    // Removing the outermost column or row would move the picture's edge, so
    // only interior lines can go. False when there is nothing removable.
    bool remove_column(int index);
    bool remove_row(int index);

    // The source coordinate of a column or row, in 0..1.
    double column_u(int col) const;
    double row_v(int row) const;

    // Installs the source coordinates wholesale -- what loading a preset does.
    // Refused unless both run strictly upwards from 0 to 1 and match the grid's
    // size: `locate` binary-searches these, and a set that doubled back or left
    // gaps would place pixels somewhere no dragging could ever explain. The
    // invariant is checked here rather than assumed from however the grid was
    // built, because a file is not a construction path.
    bool set_source_coordinates(const std::vector<double>& us,
                                const std::vector<double>& vs);

    // True while every point still sits on its even share of the unit square,
    // which is the state where the mesh can be skipped entirely.
    bool is_identity() const;

private:
    Point evaluate_bilinear(double u, double v) const;
    Point evaluate_bezier(double u, double v) const;

    int cols_ = 2;
    int rows_ = 2;
    WarpInterpolation mode_ = WarpInterpolation::Bilinear;
    std::vector<Point> points_;  // row-major, rows_ * cols_
    std::vector<double> us_;     // source coordinate of each column
    std::vector<double> vs_;     // source coordinate of each row
};

// The largest grid the editor offers. Beyond this the points are closer
// together than a finger can aim, and the tool stops helping.
inline constexpr int kMaxWarpDivisions = 16;

}  // namespace svj
