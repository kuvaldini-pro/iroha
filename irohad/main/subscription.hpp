/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_SUBSCRIPTION_HPP
#define IROHA_SUBSCRIPTION_HPP

#include <memory>

#include "subscription/common.hpp"
#include "subscription/subscriber_impl.hpp"
#include "subscription/subscription_manager.hpp"

namespace iroha {
  enum SubscriptionEngineHandlers {
    kYac = 0,
    kMetrics,
    kRequestProposal,
    kVoteProcess,
    kTotalCount
  };

  enum EventTypes {
    kOnOutcome = 0,
    kOnSynchronization,
    kOnInitialSynchronization,
    kOnCurrentRoundPeers,
    kOnRoundSwitch,
    kOnProposal,
    kOnVerifiedProposal,
    kOnProcessedHashes,
    kOnOutcomeFromYac,
    kOnOutcomeDelayed,
    kOnBlock,
    kOnInitialBlock,
    kOnBlockCreatorEvent,
    kOnFinalizedTxs,
    kOnApplyState,
    kOnNeedProposal,
    kOnNewProposal,

    // MST
    kOnStateUpdate,
    kOnPreparedBatches,
    kOnExpiredBatches,

    // YAC
    kTimer,

    // TEST
    kOnTestOperationComplete
  };

  using Subscription = subscription::SubscriptionManager<
      SubscriptionEngineHandlers::kTotalCount>;
  using SubscriptionDispatcher = typename Subscription::Dispatcher;

  template <typename ObjectType, typename... EventData>
  using BaseSubscriber = subscription::SubscriberImpl<EventTypes,
                                                      SubscriptionDispatcher,
                                                      ObjectType,
                                                      EventData...>;

  std::shared_ptr<Subscription> getSubscription();

  template <typename ObjectType, typename... EventData>
  struct SubscriberCreator {
    template <EventTypes key, SubscriptionEngineHandlers tid, typename F>
    static auto create(F &&callback) {
      auto subscriber =
          std::make_shared<BaseSubscriber<ObjectType, EventData...>>(
              getSubscription()->getEngine<EventTypes, EventData...>());
      subscriber->setCallback(
          [f{std::forward<F>(callback)}](auto /*set_id*/,
                                         auto &object,
                                         auto event_key,
                                         EventData... args) {
            assert(key == event_key);
            f(object, std::move(args)...);
          });
      subscriber->template subscribe<tid>(0, key);
      return subscriber;
    }
  };
}  // namespace iroha

#endif  // IROHA_SUBSCRIPTION_HPP
