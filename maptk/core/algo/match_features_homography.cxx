/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <maptk/core/algo/match_features_homography.h>
#include <maptk/core/exceptions/algorithm.h>
#include <maptk/core/match_set.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>

#include <iostream>


namespace maptk
{

namespace algo
{

/// Private implementation class
class match_features_homography::priv
{
public:
  /// Constructor
  priv()
  : inlier_scale(2.0)
  {
  }

  priv(const priv& other)
  : inlier_scale(other.inlier_scale)
  {
  }

  // the scale of inlier points
  double inlier_scale;
};


/// Constructor
match_features_homography
::match_features_homography()
: d_(new priv)
{
}


/// Copy Constructor
match_features_homography
::match_features_homography(const match_features_homography& other)
: d_(new priv(*other.d_))
{
}


/// Destructor
match_features_homography
::~match_features_homography()
{
}


/// Get this alg's \link maptk::config_block configuration block \endlink
config_block_sptr
match_features_homography
::get_configuration() const
{
  config_block_sptr config = algorithm::get_configuration();
  config->set_value("inlier_scale",
                    boost::lexical_cast<std::string>(d_->inlier_scale),
                    "The acceptable error distance (in pixels) between warped "
                    "and measured points to be considered an inlier match.");

  // nested algorithm configurations
  estimate_homography::get_nested_algo_configuration("homography_estimator",
                                                     config, h_estimator_);
  match_features::get_nested_algo_configuration("feature_matcher", config,
                                                matcher_);

  return config;
}


void
match_features_homography
::set_configuration(config_block_sptr in_config)
{
  // Starting with our generated config_block to ensure that assumed values are present
  // An alternative is to check for key presence before performing a get_value() call.
  config_block_sptr config = this->get_configuration();
  config->merge_config(in_config);

  // Set nested algorithm configurations
  estimate_homography::set_nested_algo_configuration("homography_estimator",
                                                     config, h_estimator_);
  match_features::set_nested_algo_configuration("feature_matcher", config,
                                                matcher_);
  d_->inlier_scale = config->get_value<double>("inlier_scale");
}

bool
match_features_homography
::check_configuration(config_block_sptr config) const
{
  return (
    estimate_homography::check_nested_algo_configuration("homography_estimator", config)
    &&
    match_features::check_nested_algo_configuration("feature_matcher", config)
  );
}


/// Match one set of features and corresponding descriptors to another
match_set_sptr
match_features_homography
::match(feature_set_sptr feat1, descriptor_set_sptr desc1,
        feature_set_sptr feat2, descriptor_set_sptr desc2) const
{
  if( !matcher_ || !h_estimator_ )
  {
    return match_set_sptr();
  }

  // compute the initial matches
  match_set_sptr init_matches = matcher_->match(feat1, desc1, feat2, desc2);

  // estimate a homography from the initial matches
  std::vector<bool> inliers;
  matrix_3x3d H = h_estimator_->estimate(feat1, feat2, init_matches,
                                         inliers, d_->inlier_scale);
  (void) H; // H not yet used, avoid compiler warning
  std::cout << "inlier ratio: "<< std::count(inliers.begin(), inliers.end(), true)
            << "/"<<inliers.size() << std::endl;

  std::vector<maptk::match> m = init_matches->matches();
  std::vector<maptk::match> inlier_m;
  for( unsigned int i=0; i<inliers.size(); ++i )
  {
    if( inliers[i] )
    {
      inlier_m.push_back(m[i]);
    }
  }

  return match_set_sptr(new simple_match_set(inlier_m));
}


} // end namespace algo

} // end namespace maptk
