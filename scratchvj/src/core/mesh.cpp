#include "core/mesh.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

// Non-uniform Catmull-Rom, in Barry-Goldman form: repeated linear
// interpolation between four points at four KNOTS. With evenly spaced knots it
// reduces to the familiar uniform spline, but the knots here are the grid
// lines' own source coordinates and those stop being even the moment a line is
// inserted. Using the uniform formula on uneven knots moves the picture by more
// than a percent of its width on the first insertion -- measured, not guessed --
// which on a wall is tens of pixels of lurch.
Point interpolate(const Point& a, const Point& b, double t) {
    return Point{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

Point barry_goldman(const Point p[4], const double t[4], double x) {
    const auto blend = [&](const Point& lo, const Point& hi, double t_lo, double t_hi) {
        const double span = t_hi - t_lo;
        return interpolate(lo, hi, span > 1e-12 ? (x - t_lo) / span : 0.0);
    };
    const Point a1 = blend(p[0], p[1], t[0], t[1]);
    const Point a2 = blend(p[1], p[2], t[1], t[2]);
    const Point a3 = blend(p[2], p[3], t[2], t[3]);
    const Point b1 = blend(a1, a2, t[0], t[2]);
    const Point b2 = blend(a2, a3, t[1], t[3]);
    return blend(b1, b2, t[1], t[2]);
}

// A knot outside the stored range, continued at the spacing of the edge cell.
// Duplicating the boundary knot instead would divide by zero in the blend, and
// a spline that flared at its edges would be unusable for what this is for.
double knot_at(const std::vector<double>& knots, int index) {
    const int last = static_cast<int>(knots.size()) - 1;
    if (index < 0) {
        return knots.front() - (knots[1] - knots.front()) * -index;
    }
    if (index > last) {
        return knots.back() +
               (knots.back() - knots[static_cast<std::size_t>(last) - 1]) * (index - last);
    }
    return knots[static_cast<std::size_t>(index)];
}

// The cell a source coordinate falls in, and how far across it. The lines are
// NOT evenly spaced -- that is the point of storing their coordinates -- so the
// span is searched rather than divided.
void locate(const std::vector<double>& coordinates, double value, int& cell, double& t) {
    const int last = static_cast<int>(coordinates.size()) - 1;
    const double clamped = std::clamp(value, coordinates.front(), coordinates.back());
    cell = 0;
    while (cell < last - 1 && clamped > coordinates[static_cast<std::size_t>(cell) + 1]) {
        ++cell;
    }
    const double lo = coordinates[static_cast<std::size_t>(cell)];
    const double hi = coordinates[static_cast<std::size_t>(cell) + 1];
    t = hi - lo > 1e-12 ? (clamped - lo) / (hi - lo) : 0.0;
}

}  // namespace

void WarpMesh::reset(int cols, int rows) {
    cols_ = std::clamp(cols, 2, kMaxWarpDivisions);
    rows_ = std::clamp(rows, 2, kMaxWarpDivisions);
    points_.resize(static_cast<std::size_t>(cols_) * rows_);
    us_.resize(static_cast<std::size_t>(cols_));
    vs_.resize(static_cast<std::size_t>(rows_));

    for (int col = 0; col < cols_; ++col) {
        us_[static_cast<std::size_t>(col)] = static_cast<double>(col) / (cols_ - 1);
    }
    for (int row = 0; row < rows_; ++row) {
        vs_[static_cast<std::size_t>(row)] = static_cast<double>(row) / (rows_ - 1);
    }
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            points_[static_cast<std::size_t>(row) * cols_ + col] =
                Point{us_[static_cast<std::size_t>(col)], vs_[static_cast<std::size_t>(row)]};
        }
    }
}

const Point& WarpMesh::at(int col, int row) const {
    static const Point kOrigin{0.0, 0.0};
    if (points_.empty()) return kOrigin;
    const int c = std::clamp(col, 0, cols_ - 1);
    const int r = std::clamp(row, 0, rows_ - 1);
    return points_[static_cast<std::size_t>(r) * cols_ + c];
}

void WarpMesh::set(int col, int row, Point value) {
    if (points_.empty()) return;
    if (col < 0 || col >= cols_ || row < 0 || row >= rows_) return;
    points_[static_cast<std::size_t>(row) * cols_ + col] = value;
}

double WarpMesh::column_u(int col) const {
    if (us_.empty()) return 0.0;
    return us_[static_cast<std::size_t>(std::clamp(col, 0, cols_ - 1))];
}

double WarpMesh::row_v(int row) const {
    if (vs_.empty()) return 0.0;
    return vs_[static_cast<std::size_t>(std::clamp(row, 0, rows_ - 1))];
}

bool WarpMesh::set_source_coordinates(const std::vector<double>& us,
                                      const std::vector<double>& vs) {
    const auto valid = [](const std::vector<double>& c, int expected) {
        if (static_cast<int>(c.size()) != expected) return false;
        if (std::fabs(c.front()) > 1e-9 || std::fabs(c.back() - 1.0) > 1e-9) return false;
        for (std::size_t i = 1; i < c.size(); ++i) {
            if (c[i] <= c[i - 1]) return false;
        }
        return true;
    };
    if (!valid(us, cols_) || !valid(vs, rows_)) return false;
    us_ = us;
    vs_ = vs;
    return true;
}

Point WarpMesh::evaluate_bilinear(double u, double v) const {
    int c = 0, r = 0;
    double tu = 0.0, tv = 0.0;
    locate(us_, u, c, tu);
    locate(vs_, v, r, tv);

    const Point top = interpolate(at(c, r), at(c + 1, r), tu);
    const Point bottom = interpolate(at(c, r + 1), at(c + 1, r + 1), tu);
    return interpolate(top, bottom, tv);
}

Point WarpMesh::evaluate_bezier(double u, double v) const {
    int c = 0, r = 0;
    double unused = 0.0;
    locate(us_, u, c, unused);
    locate(vs_, v, r, unused);
    const double cu = std::clamp(u, us_.front(), us_.back());
    const double cv = std::clamp(v, vs_.front(), vs_.back());

    // The tensor product: four rows curved along u at the real u, then those
    // four results curved along v. The row knots come from vs_, so the second
    // pass respects the spacing of the rows just as the first respects the
    // columns.
    const double u_knots[4] = {knot_at(us_, c - 1), knot_at(us_, c), knot_at(us_, c + 1),
                               knot_at(us_, c + 2)};
    Point column[4];
    for (int k = 0; k < 4; ++k) {
        const int row = r - 1 + k;
        const Point row_points[4] = {at(c - 1, row), at(c, row), at(c + 1, row),
                                     at(c + 2, row)};
        column[k] = barry_goldman(row_points, u_knots, cu);
    }
    const double v_knots[4] = {knot_at(vs_, r - 1), knot_at(vs_, r), knot_at(vs_, r + 1),
                               knot_at(vs_, r + 2)};
    return barry_goldman(column, v_knots, cv);
}

Point WarpMesh::map(double u, double v) const {
    if (points_.empty()) return Point{u, v};
    return mode_ == WarpInterpolation::Bezier ? evaluate_bezier(u, v)
                                              : evaluate_bilinear(u, v);
}

bool WarpMesh::insert_column(int before) {
    if (cols_ >= kMaxWarpDivisions) return false;
    const int index = std::clamp(before, 1, cols_ - 1);

    // The new line takes the midpoint of the two source coordinates it splits,
    // and its points are SAMPLED from the surface as it stands -- so the image
    // each existing line carries is untouched and the picture does not move.
    const double u = 0.5 * (us_[static_cast<std::size_t>(index) - 1] +
                            us_[static_cast<std::size_t>(index)]);
    std::vector<Point> inserted(static_cast<std::size_t>(rows_));
    for (int row = 0; row < rows_; ++row) {
        inserted[static_cast<std::size_t>(row)] = map(u, vs_[static_cast<std::size_t>(row)]);
    }

    std::vector<Point> grown(static_cast<std::size_t>(cols_ + 1) * rows_);
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < index; ++col) {
            grown[static_cast<std::size_t>(row) * (cols_ + 1) + col] = at(col, row);
        }
        grown[static_cast<std::size_t>(row) * (cols_ + 1) + index] =
            inserted[static_cast<std::size_t>(row)];
        for (int col = index; col < cols_; ++col) {
            grown[static_cast<std::size_t>(row) * (cols_ + 1) + col + 1] = at(col, row);
        }
    }
    points_ = std::move(grown);
    us_.insert(us_.begin() + index, u);
    ++cols_;
    return true;
}

bool WarpMesh::insert_row(int before) {
    if (rows_ >= kMaxWarpDivisions) return false;
    const int index = std::clamp(before, 1, rows_ - 1);

    const double v = 0.5 * (vs_[static_cast<std::size_t>(index) - 1] +
                            vs_[static_cast<std::size_t>(index)]);
    std::vector<Point> inserted(static_cast<std::size_t>(cols_));
    for (int col = 0; col < cols_; ++col) {
        inserted[static_cast<std::size_t>(col)] = map(us_[static_cast<std::size_t>(col)], v);
    }

    std::vector<Point> grown(static_cast<std::size_t>(cols_) * (rows_ + 1));
    for (int row = 0; row < index; ++row) {
        for (int col = 0; col < cols_; ++col) {
            grown[static_cast<std::size_t>(row) * cols_ + col] = at(col, row);
        }
    }
    for (int col = 0; col < cols_; ++col) {
        grown[static_cast<std::size_t>(index) * cols_ + col] =
            inserted[static_cast<std::size_t>(col)];
    }
    for (int row = index; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            grown[static_cast<std::size_t>(row + 1) * cols_ + col] = at(col, row);
        }
    }
    points_ = std::move(grown);
    vs_.insert(vs_.begin() + index, v);
    ++rows_;
    return true;
}

bool WarpMesh::remove_column(int index) {
    if (cols_ <= 2 || index <= 0 || index >= cols_ - 1) return false;
    std::vector<Point> shrunk(static_cast<std::size_t>(cols_ - 1) * rows_);
    for (int row = 0; row < rows_; ++row) {
        int write = 0;
        for (int col = 0; col < cols_; ++col) {
            if (col == index) continue;
            shrunk[static_cast<std::size_t>(row) * (cols_ - 1) + write++] = at(col, row);
        }
    }
    points_ = std::move(shrunk);
    us_.erase(us_.begin() + index);
    --cols_;
    return true;
}

bool WarpMesh::remove_row(int index) {
    if (rows_ <= 2 || index <= 0 || index >= rows_ - 1) return false;
    std::vector<Point> shrunk(static_cast<std::size_t>(cols_) * (rows_ - 1));
    int write = 0;
    for (int row = 0; row < rows_; ++row) {
        if (row == index) continue;
        for (int col = 0; col < cols_; ++col) {
            shrunk[static_cast<std::size_t>(write) * cols_ + col] = at(col, row);
        }
        ++write;
    }
    points_ = std::move(shrunk);
    vs_.erase(vs_.begin() + index);
    --rows_;
    return true;
}

bool WarpMesh::is_identity() const {
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const Point& actual = at(col, row);
            if (std::fabs(actual.x - us_[static_cast<std::size_t>(col)]) > 1e-9 ||
                std::fabs(actual.y - vs_[static_cast<std::size_t>(row)]) > 1e-9) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace svj
