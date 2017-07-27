/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include "consensus/yac/storage/yac_vote_storage.hpp"
#include "yac_mocks.hpp"

using namespace iroha::consensus::yac;

TEST(YacStorageTest, SupermajorityFunctionForAllCases) {
  int N = 4;
  ASSERT_EQ(false, hasSupermajority(0, N));
  ASSERT_EQ(false, hasSupermajority(1, N));
  ASSERT_EQ(false, hasSupermajority(2, N));
  ASSERT_EQ(true, hasSupermajority(3, N));
  ASSERT_EQ(true, hasSupermajority(4, N));
  ASSERT_EQ(false, hasSupermajority(5, N));
}

TEST(YacStorageTest, SupermajorityFunctionForAllCases2) {
  int N = 2;
  ASSERT_EQ(false, hasSupermajority(0, N));
  ASSERT_EQ(false, hasSupermajority(1, N));
  ASSERT_EQ(true, hasSupermajority(2, N));
  ASSERT_EQ(false, hasSupermajority(3, N));
}

TEST(YacStorageTest, YacBlockStorageWhenNormalDataInput) {
  YacHash hash("proposal", "commit");
  int N = 4;
  YacBlockStorage storage(hash.proposal_hash, hash.block_hash, N);

  auto insert_1 = storage.insert(create_vote(hash, "one"));
  ASSERT_EQ(true, insert_1.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_1.commit);
  ASSERT_EQ(nonstd::nullopt, insert_1.reject);

  auto insert_2 = storage.insert(create_vote(hash, "two"));
  ASSERT_EQ(true, insert_2.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_2.commit);
  ASSERT_EQ(nonstd::nullopt, insert_2.reject);

  auto insert_3 = storage.insert(create_vote(hash, "three"));
  ASSERT_EQ(true, insert_3.vote_inserted);
  ASSERT_EQ(3, insert_3.commit->votes.size());
  ASSERT_EQ(nonstd::nullopt, insert_3.reject);

  auto insert_4 = storage.insert(create_vote(hash, "four"));
  ASSERT_EQ(false, insert_4.vote_inserted);
  ASSERT_EQ(3, insert_4.commit->votes.size());
  ASSERT_EQ(nonstd::nullopt, insert_4.reject);
}

TEST(YacStorageTest, YacProposalStorageWhenNormalDataInput) {
  YacHash hash("proposal", "commit");
  int N = 4;
  YacProposalStorage storage(hash.proposal_hash, N);

  auto insert_1 = storage.insert(create_vote(hash, "one"));
  ASSERT_EQ(true, insert_1.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_1.commit);
  ASSERT_EQ(nonstd::nullopt, insert_1.reject);

  auto insert_2 = storage.insert(create_vote(hash, "two"));
  ASSERT_EQ(true, insert_2.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_2.commit);
  ASSERT_EQ(nonstd::nullopt, insert_2.reject);

  auto insert_3 = storage.insert(create_vote(hash, "three"));
  ASSERT_EQ(true, insert_3.vote_inserted);
  ASSERT_NE(nonstd::nullopt, insert_3.commit);
  ASSERT_EQ(nonstd::nullopt, insert_3.reject);

  auto insert_4 = storage.insert(create_vote(hash, "four"));
  ASSERT_EQ(false, insert_4.vote_inserted);
  ASSERT_NE(nonstd::nullopt, insert_4.commit);
  ASSERT_EQ(nonstd::nullopt, insert_4.reject);
}

TEST(YacStorageTest, YacStorageWhenNormalDataInput) {
  YacHash hash("proposal", "commit");
  int N = 4;
  YacVoteStorage storage;

  auto insert_1 = storage.storeVote(create_vote(hash, "one"), 4);
  ASSERT_EQ(true, insert_1.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_1.commit);
  ASSERT_EQ(nonstd::nullopt, insert_1.reject);

  auto insert_2 = storage.storeVote(create_vote(hash, "two"), 4);
  ASSERT_EQ(true, insert_2.vote_inserted);
  ASSERT_EQ(nonstd::nullopt, insert_2.commit);
  ASSERT_EQ(nonstd::nullopt, insert_2.reject);

  auto insert_3 = storage.storeVote(create_vote(hash, "three"), 4);
  ASSERT_EQ(true, insert_3.vote_inserted);
  ASSERT_NE(nonstd::nullopt, insert_3.commit);
  ASSERT_EQ(nonstd::nullopt, insert_3.reject);

  auto insert_4 = storage.storeVote(create_vote(hash, "four"), 4);
  ASSERT_EQ(false, insert_4.vote_inserted);
  ASSERT_NE(nonstd::nullopt, insert_4.commit);
  ASSERT_EQ(nonstd::nullopt, insert_4.reject);
}