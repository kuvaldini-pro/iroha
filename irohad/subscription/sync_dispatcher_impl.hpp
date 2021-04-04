/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_SUBSCRIPTION_SYNC_DISPATCHER_IMPL_HPP
#define IROHA_SUBSCRIPTION_SYNC_DISPATCHER_IMPL_HPP

#include "subscription/dispatcher.hpp"

#include "subscription/common.hpp"
#include "subscription/thread_handler.hpp"

namespace iroha::subscription {

  template <uint32_t kCount, uint32_t kPoolSize>
  class SyncDispatcher final : public IDispatcher<kCount, kPoolSize>,
                               utils::NoCopy,
                               utils::NoMove {
   private:
    using Parent = IDispatcher<kCount, kPoolSize>;

   public:
    SyncDispatcher() = default;

    void dispose() override {}

    void add(typename Parent::Tid /*tid*/,
             typename Parent::Task &&task) override {
      task();
    }

    void addDelayed(typename Parent::Tid /*tid*/,
                    std::chrono::microseconds /*timeout*/,
                    typename Parent::Task &&task) override {
      task();
    }
  };

}  // namespace iroha::subscription

#endif  // IROHA_SUBSCRIPTION_SYNC_DISPATCHER_IMPL_HPP