//===----------------------------------------------------------------------===//
//
//                         PelotonDB
//
// clusterer.cpp
//
// Identification: src/backend/brain/clusterer.cpp
//
// Copyright (c) 2015, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#include <limits>
#include <sstream>
#include <iostream>
#include <map>
#include <cassert>

#include "backend/brain/clusterer.h"
#include "backend/common/logger.h"

namespace peloton {
namespace brain {

// http://www.cs.princeton.edu/courses/archive/fall08/cos436/Duda/C/sk_means.htm
void Clusterer::ProcessSample(const Sample &sample) {
  // Figure out closest cluster
  oid_t closest_cluster = GetClosestCluster(sample);

  Sample distance = sample.GetDifference(means_[closest_cluster]);
  Sample mean_drift = distance * new_sample_weight_;

  // std::cout << "mean drift : " << mean_drift << "\n";

  // Update the cluster's mean
  means_[closest_cluster] = means_[closest_cluster] + mean_drift;
}

oid_t Clusterer::GetClosestCluster(const Sample &sample) {
  double min_dist = std::numeric_limits<double>::max();
  oid_t closest_cluster = START_OID;
  oid_t cluster_itr = START_OID;

  // Go over all the means and find closest cluster
  for (auto mean : means_) {
    auto dist = sample.GetDistance(mean);
    if (dist < min_dist) {
      closest_cluster = cluster_itr;
      min_dist = dist;
    }
    cluster_itr++;
  }

  closest_[closest_cluster]++;
  sample_count_++;

  return closest_cluster;
}

Sample Clusterer::GetCluster(oid_t cluster_offset) const {
  return means_[cluster_offset];
}

double Clusterer::GetFraction(oid_t cluster_offset) const {
  return ((double)closest_[cluster_offset]) / sample_count_;
}

std::map<oid_t, oid_t> Clusterer::GetPartitioning(oid_t tile_count) const {
  assert(tile_count >= 1);
  assert(tile_count <= sample_column_count_);

  std::vector<oid_t> partitioning;

  std::map<double, oid_t> frequencies;
  oid_t cluster_itr = START_OID;
  oid_t cluster_count;

  cluster_count = GetClusterCount();
  for (cluster_itr = 0; cluster_itr < cluster_count; cluster_itr++) {
    auto pair = std::make_pair(GetFraction(cluster_itr), cluster_itr);
    frequencies.insert(pair);
  }

  std::map<oid_t, oid_t> column_to_tile_map;
  oid_t tile_itr = START_OID;
  oid_t remaining_column_count = sample_column_count_;

  // look for most significant cluster
  for (auto entry = frequencies.rbegin(); entry != frequencies.rend();
       ++entry) {
    LOG_TRACE(" %u :: %.3lf", entry->second, entry->first);

    // first, check if remaining columns less than tile count
    if (remaining_column_count <= tile_count) {
      oid_t column_itr;
      for (column_itr = 0; column_itr < sample_column_count_; column_itr++) {
        if (column_to_tile_map.count(column_itr) == 0) {
          column_to_tile_map[column_itr] = tile_itr;
          tile_itr++;
        }
      }
    }

    // otherwise, get its partitioning
    auto config = means_[entry->second];
    auto config_tile = config.GetEnabledColumns();

    for (auto column : config_tile) {
      if (column_to_tile_map.count(column) == 0) {
        column_to_tile_map[column] = tile_itr;
        remaining_column_count--;
      }
    }

    // check tile itr
    tile_itr++;
    if (tile_itr >= tile_count)
      tile_itr--;
  }

  // check if all columns are present in partitioning
  assert(column_to_tile_map.size() == sample_column_count_);

  return column_to_tile_map;
}

std::ostream &operator<<(std::ostream &os, const Clusterer &clusterer) {
  oid_t cluster_itr;
  oid_t cluster_count;

  cluster_count = clusterer.GetClusterCount();
  for (cluster_itr = 0; cluster_itr < cluster_count; cluster_itr++)
    os << cluster_itr << " : " << clusterer.GetFraction(cluster_itr)
       << " :: " << clusterer.GetCluster(cluster_itr);

  return os;
}

} // End brain namespace
} // End peloton namespace