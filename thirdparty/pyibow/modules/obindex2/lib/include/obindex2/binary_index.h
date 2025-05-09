/**
* This file is part of obindex2.
*
* Copyright (C) 2017 Emilio Garcia-Fidalgo <emilio.garcia@uib.es> (University of the Balearic Islands)
*
* obindex2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* obindex2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with obindex2. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIB_INCLUDE_OBINDEX2_BINARY_INDEX_H_
#define LIB_INCLUDE_OBINDEX2_BINARY_INDEX_H_

#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "obindex2/binary_tree.h"

#include "obindex2/BoostArchiver.h"

namespace obindex2 {

enum MergePolicy {
  MERGE_POLICY_NONE,
  MERGE_POLICY_AND,
  MERGE_POLICY_OR
};

struct InvIndexItem {
  InvIndexItem() :
      image_id(0),
      pt(0.0f, 0.0f),
      dist(DBL_MAX),
      kp_ind(-1) {}

  InvIndexItem(const int id,
               const cv::Point2f kp,
               const double d,
               const int kp_i = -1) :
  image_id(id),
  pt(kp),
  dist(d),
  kp_ind(kp_i)
  {}

  unsigned image_id;
  cv::Point2f pt;
  double dist;
  int kp_ind;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & image_id;
    ar & pt;
    ar & dist;
    ar & kp_ind;
  }
};

struct ImageMatch {
  ImageMatch() :
      image_id(-1),
      score(0.0) {}

  explicit ImageMatch(const int id, const double sc = 0.0) :
      image_id(id),
      score(sc) {}

  int image_id;
  double score;

  bool operator<(const ImageMatch &lcr) const { return score > lcr.score; }

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & image_id;
    ar & score;
  }
};

struct PointMatches {
  std::vector<cv::Point2f> query;
  std::vector<cv::Point2f> train;
};

class ImageIndex {
 public:
  // Constructors
  //ImageIndex();
  ImageIndex(const unsigned k = 16,
             const unsigned s = 150,
             const unsigned t = 4,
             const MergePolicy merge_policy = MERGE_POLICY_NONE,
             const bool purge_descriptors = true,
             const unsigned min_feat_apps = 3);

  // Methods
  void addImage(const unsigned image_id,
                const std::vector<cv::KeyPoint>& kps,
                const cv::Mat& descs);
  void addImage(const unsigned image_id,
                const std::vector<cv::KeyPoint>& kps,
                const cv::Mat& descs,
                const std::vector<cv::DMatch>& matches);
  void searchImages(const cv::Mat& descs,
                    const std::vector<cv::DMatch>& gmatches,
                    std::vector<ImageMatch>* img_matches,
                    bool sort = true);
  void searchDescriptors(const cv::Mat& descs,
                         std::vector<std::vector<cv::DMatch> >* matches,
                         const unsigned knn = 2,
                         const unsigned checks = 32);
  void deleteDescriptor(const unsigned desc_id);
  void getMatchings(const std::vector<cv::KeyPoint>& query_kps,
                    const std::vector<cv::DMatch>& matches,
                    std::unordered_map<unsigned, PointMatches>* point_matches);
  inline unsigned numImages() const {
    return nimages_;
  }

  inline unsigned numDescriptors() const {
    return dset_.size();
  }

  inline void rebuild() {
    if (init_) {
      trees_.clear();
      initTrees();
    }
  }

  inline void clear() {
    init_ = false;
    nimages_ = 0;
    ndesc_ = 0;

    dset_.clear();
    trees_.clear();
    inv_index_.clear();
    desc_to_id_.clear();
    id_to_desc_.clear();
    recently_added_.clear();
  }


  void save(const std::string& filename);
  void load(const std::string& filename);

  void printStatus() const;

  friend std::ostream& operator<<(std::ostream &os, const ImageIndex &db);  

 private:
  BinaryDescriptorSet dset_;
  unsigned k_;
  unsigned s_;
  unsigned t_;
  unsigned init_ = false;
  unsigned nimages_ = 0;
  unsigned ndesc_ = 0;
  MergePolicy merge_policy_;
  bool purge_descriptors_;
  unsigned min_feat_apps_;

  std::vector<BinaryTreePtr> trees_;
  std::unordered_map<BinaryDescriptorPtr, std::vector<InvIndexItem> > inv_index_;
  std::unordered_map<BinaryDescriptorPtr, unsigned> desc_to_id_;
  std::unordered_map<unsigned, BinaryDescriptorPtr> id_to_desc_;
  std::list<BinaryDescriptorPtr> recently_added_;

 private: 

  void initTrees();
  void searchDescriptor(BinaryDescriptorPtr q,
                        std::vector<BinaryDescriptorPtr>* neigh,
                        std::vector<double>* distances,
                        unsigned knn = 2,
                        unsigned checks = 32);
  void insertDescriptor(BinaryDescriptorPtr q);
  void deleteDescriptor(BinaryDescriptorPtr q);
  void purgeDescriptors(const unsigned curr_img);

 protected: 

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & dset_;
    ar & k_;
    ar & s_;
    ar & t_;
    ar & init_;
    ar & nimages_;
    ar & ndesc_;
    ar & merge_policy_;
    ar & purge_descriptors_;
    ar & min_feat_apps_;

    ar & trees_;
    ar & inv_index_;
    ar & desc_to_id_;
    ar & id_to_desc_;
    ar & recently_added_;
  }

};

}  // namespace obindex2

#endif  // LIB_INCLUDE_OBINDEX2_BINARY_INDEX_H_
