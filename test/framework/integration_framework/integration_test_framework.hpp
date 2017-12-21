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

#ifndef IROHA_INTEGRATION_FRAMEWORK_HPP
#define IROHA_INTEGRATION_FRAMEWORK_HPP

#include <tbb/concurrent_queue.h>
#include <chrono>
#include <exception>
#include <queue>
#include <thread>
#include "framework/integration_framework/iroha_instance.hpp"
#include "logger/logger.hpp"
#include "model/block.hpp"
#include "model/proposal.hpp"
#include "model/transaction.hpp"

namespace integration_framework {

  using namespace std::chrono_literals;

  class IntegrationTestFramework {
   private:
    using ProposalType = std::shared_ptr<iroha::model::Proposal>;
    using BlockType = std::shared_ptr<iroha::model::Block>;

   public:
    // init
    IntegrationTestFramework &setInitialState(
        const iroha::keypair_t &keypair, const iroha::model::Block &block) {
      log_->info("init state");
      // peer initialization
      iroha_instance_->initPipeline(keypair);
      log_->info("created pipeline");
      iroha_instance_->clearLedger();
      log_->info("cleared ledger");
      iroha_instance_->rawInsertBlock(block);
      log_->info("raw insert");

      // subscribing for components

      iroha_instance_->getIrohaInstance()
          ->getPeerCommunicationService()
          ->on_proposal()
          .subscribe([this](auto proposal) {
            proposal_queue_.push(
                std::make_shared<iroha::model::Proposal>(proposal));
          });

      iroha_instance_->getIrohaInstance()
          ->getPeerCommunicationService()
          ->on_commit()
          .subscribe([this](auto commit_observable) {
            commit_observable.subscribe([this](auto committed_block) {
              block_queue_.push(
                  std::make_shared<iroha::model::Block>(committed_block));
            });
          });

      // start instance
      iroha_instance_->run();
      log_->info("run iroha");
      return *this;
    }

    // send tx
    IntegrationTestFramework &sendTx(const iroha::model::Transaction &tx) {
      sendTx(tx, [] {});
      return *this;
    }

    template <typename Lambda>
    IntegrationTestFramework &sendTx(const iroha::model::Transaction &tx,
                                     Lambda validation) {
      log_->info("send transaction");
      // deserialize
      auto pb_tx =
          iroha::model::converters::PbTransactionFactory().serialize(tx);
      // send
      google::protobuf::Empty response;
      iroha_instance_->getIrohaInstance()->getCommandService()->ToriiAsync(
          pb_tx, response);
      // fetch status of transaction
      // check validation function
      return *this;
    }

    // proposal
    template <typename Lambda>
    IntegrationTestFramework &checkProposal(Lambda validation) {
      log_->info("check proposal");
      // fetch first proposal from proposal queue
      ProposalType proposal;
      fetchFromQueue(
          proposal_queue_, proposal, proposal_waiting, "missed proposal");
      return *this;
    }

    IntegrationTestFramework &skipProposal() {
      checkProposal([](const iroha::model::Proposal &) {});
      return *this;
    }

    // block
    template <typename Lambda>
    IntegrationTestFramework &checkBlock(Lambda validation) {
      // fetch first from block queue
      log_->info("check block");
      BlockType block;
      fetchFromQueue(block_queue_, block, block_waiting, "missed block");
      validation(*block);
      return *this;
    }
    IntegrationTestFramework &skipBlock() {
      checkBlock([](const iroha::model::Block &) {});
      return *this;
    }

    // shutdown iroha
    void done() {
      log_->info("done");
      iroha_instance_->clearLedger();
    }

    /**
     * general way to fetch object from concurrent queue
     * @tparam Queue - Type of queue
     * @tparam ObjectType - Type of fetched object
     * @tparam WaitTime - time for waiting if data doesn't appear
     * @param queue - queue instance for fetching
     * @param ref_for_insertion - reference to insert object
     * @param wait - time of waiting
     * @param error_reason - reason if thehre is no appeared object at all
     */
    template <typename Queue, typename ObjectType, typename WaitTime>
    static void fetchFromQueue(Queue &queue,
                               ObjectType &ref_for_insertion,
                               const WaitTime &wait,
                               const std::string &error_reason) {
      if (!queue.try_pop(ref_for_insertion)) {
        std::this_thread::sleep_for(wait);
      }
      if (!queue.try_pop(ref_for_insertion)) {
        throw std::runtime_error(error_reason);
      }
    }

   protected:
    std::shared_ptr<IrohaInstance> iroha_instance_ =
        std::make_shared<IrohaInstance>();
    tbb::concurrent_queue<ProposalType> proposal_queue_;
    tbb::concurrent_queue<BlockType> block_queue_;

    // config area

    /// maximum time of waiting before appearing next proposal
    // TODO 21/12/2017 muratovv make relation of time with instance's config
    const std::chrono::milliseconds proposal_waiting = 5000ms;

    /// maximum time of waiting before appearing next committed block
    const std::chrono::milliseconds block_waiting = 5000ms;

   private:
    logger::Logger log_ = logger::log("IntegrationTestFramework");
  };
}  // namespace integration_framework

#endif  // IROHA_INTEGRATION_FRAMEWORK_HPP
