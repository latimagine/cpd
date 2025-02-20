// cpd - Coherent Point Drift
// Copyright (C) 2017 Pete Gadomski <pete.gadomski@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/// \file
///
/// Nonrigid coherent point drift transform.

#pragma once

#include <cpd/transform.hpp>

namespace cpd {

/// Default value for beta.
const double DEFAULT_BETA = 3.0;
/// Default value for lambda.
const double DEFAULT_LAMBDA = 3.0;

/// The result of a nonrigid coherent point drift run.
struct NonrigidResult : public Result {};

/// Nonrigid coherent point drift.

enum class NonrigidPolicy {
  PRECISION = 0,
  PERFORMANCE
};

/// Non-Rigid coherent point drift.
///
/// NOTE: Modified by Latim to enable performance vs precision
///       computations
///
template <NonrigidPolicy P = NonrigidPolicy::PRECISION>
class Nonrigid : public Transform<NonrigidResult> {
public:
    Nonrigid()
      : Transform()
      , m_lambda(DEFAULT_LAMBDA)
      , m_beta(DEFAULT_BETA)
      , m_linked(DEFAULT_LINKED) {}

    /// Initialize this transform for the provided matrices.
    void init(const Matrix& fixed, const Matrix& moving) {
      m_g = affinity(moving, moving, m_beta);
      m_w = Matrix::Zero(moving.rows(), moving.cols());
    }

    /// Modifies the probabilities with some affinity and weight information.
    void modify_probabilities(Probabilities& probabilities) const {
      probabilities.l += m_lambda / 2.0 * (m_w.transpose() * m_g * m_w).trace();
    }

    /// Sets the beta.
    Nonrigid& beta(double beta) {
        m_beta = beta;
        return *this;
    }

    /// Sets the lambda.
    Nonrigid& lambda(double lambda) {
        m_lambda = lambda;
        return *this;
    }

    /// Computes one iteration of the nonrigid transformation.
    NonrigidResult compute_one(const Matrix& fixed, const Matrix& moving,
                               const Probabilities& probabilities,
                               double sigma2) const {
                                size_t cols = fixed.cols();
      auto dp = probabilities.p1.asDiagonal();
      
      Matrix w;
      if constexpr (P == NonrigidPolicy::PRECISION) {
        w = (dp * m_g + m_lambda * sigma2 *
                                  Matrix::Identity(moving.rows(), moving.rows()))
                      .colPivHouseholderQr()
                      .solve(probabilities.px - dp * moving);
      } else {
        w = (dp * m_g + m_lambda * sigma2 *
                                Matrix::Identity(moving.rows(), moving.rows()))
                    .householderQr()
                    .solve(probabilities.px - dp * moving);
      }
      NonrigidResult result;
      result.points = moving + m_g * w;
      double np = probabilities.p1.sum();
      result.sigma2 = std::abs(
          ((fixed.array().pow(2) * probabilities.pt1.replicate(1, cols).array())
              .sum() +
          (result.points.array().pow(2) *
            probabilities.p1.replicate(1, cols).array())
              .sum() -
          2 * (probabilities.px.transpose() * result.points).trace()) /
          (np * cols));
      return result;
    }

    /// Sets whether the scalings of the two datasets are linked.
    Nonrigid& linked(bool linked) {
        m_linked = linked;
        return *this;
    }

    virtual bool linked() const { return m_linked; }

private:
    Matrix m_g;
    Matrix m_w;
    double m_lambda;
    double m_beta;
    bool m_linked;
};

/// Runs a nonrigid registration on two matrices (precision)
NonrigidResult nonrigid(const Matrix& fixed, const Matrix& moving);

/// Runs a nonrigid registration on two matrices (performance)
NonrigidResult nonrigid_quick(const Matrix& fixed, const Matrix& moving);
} // namespace cpd
