/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_SUBSCRIPTION_SUBSCRIPTION_ENGINE_HPP
#define IROHA_SUBSCRIPTION_SUBSCRIPTION_ENGINE_HPP

#include <list>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "subscription/common.hpp"
#include "subscription/dispatcher.hpp"

namespace iroha::subscription {

  template <typename Event,
            typename Dispatcher,
            typename Receiver,
            typename... Arguments>
  class Subscriber;

  using SubscriptionSetId = uint32_t;

  /**
   * @tparam EventKey - the type of a specific event from event set (e. g. a key
   * from a storage or a particular kind of event from an enumeration)
   * @tparam Receiver - the type of an object that is a part of a Subscriber
   * internal state and can be accessed on every event
   * @tparam EventParams - set of types of values passed on each event
   * notification
   */
  template <typename EventKey, typename Dispatcher, typename Receiver>
  class SubscriptionEngine final
      : public std::enable_shared_from_this<
            SubscriptionEngine<EventKey, Dispatcher, Receiver>>,
        utils::NoMove,
        utils::NoCopy {
   public:
    using EventKeyType = EventKey;
    using ReceiverType = Receiver;
    using SubscriberType = Receiver;
    using SubscriberWeakPtr = std::weak_ptr<SubscriberType>;
    using DispatcherType = typename std::decay<Dispatcher>::type;
    using DispatcherPtr = std::shared_ptr<DispatcherType>;

    /// List is preferable here because this container iterators remain
    /// alive after removal from the middle of the container
    /// using custom allocator
    using SubscribersContainer = std::list<std::tuple<typename Dispatcher::Tid,
                                                      SubscriptionSetId,
                                                      SubscriberWeakPtr>>;
    using IteratorType = typename SubscribersContainer::iterator;

   public:
    explicit SubscriptionEngine(DispatcherPtr const &dispatcher)
        : dispatcher_(dispatcher) {
      assert(dispatcher_);
    }
    ~SubscriptionEngine() = default;

   private:
    template <typename KeyType,
              typename DispatcherType,
              typename ValueType,
              typename... Args>
    friend class Subscriber;
    using KeyValueContainer =
        std::unordered_map<EventKeyType, SubscribersContainer>;

    mutable std::shared_mutex subscribers_map_cs_;
    KeyValueContainer subscribers_map_;
    DispatcherPtr dispatcher_;

    template <typename Dispatcher::Tid kTid>
    IteratorType subscribe(SubscriptionSetId set_id,
                           const EventKeyType &key,
                           SubscriberWeakPtr ptr) {
      Dispatcher::template checkTid<kTid>();
      std::unique_lock lock(subscribers_map_cs_);
      auto &subscribers_list = subscribers_map_[key];
      return subscribers_list.emplace(
          subscribers_list.end(),
          std::make_tuple(kTid, set_id, std::move(ptr)));
    }

    void unsubscribe(const EventKeyType &key, const IteratorType &it_remove) {
      std::unique_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() != it) {
        it->second.erase(it_remove);
        if (it->second.empty())
          subscribers_map_.erase(it);
      }
    }

   public:
    size_t size(const EventKeyType &key) const {
      std::shared_lock lock(subscribers_map_cs_);
      if (auto it = subscribers_map_.find(key); it != subscribers_map_.end())
        return it->second.size();

      return 0ull;
    }

    size_t size() const {
      std::shared_lock lock(subscribers_map_cs_);
      size_t count = 0ull;
      for (auto &it : subscribers_map_) count += it.second.size();
      return count;
    }

    template <typename... EventParams>
    void notify(const EventKeyType &key, EventParams... args) {
      std::shared_lock lock(subscribers_map_cs_);
      auto it = subscribers_map_.find(key);
      if (subscribers_map_.end() == it)
        return;

      auto &subscribers_container = it->second;
      for (auto it_sub = subscribers_container.begin();
           it_sub != subscribers_container.end();) {
        auto wsub = std::get<2>(*it_sub);
        auto id = std::get<1>(*it_sub);

        if (auto sub = wsub.lock()) {
          dispatcher_->add(std::get<0>(*it_sub),
                           [wsub(std::move(wsub)),
                            id(id),
                            key(key),
                            args = std::make_tuple(args...)]() {
                             if (auto sub = wsub.lock())
                               std::apply(
                                   [&](auto &&... args) {
                                     sub->on_notify(id, key, args...);
                                   },
                                   std::move(args));
                           });
          ++it_sub;
        } else {
          it_sub = subscribers_container.erase(it_sub);
        }
      }
    }
  };

}  // namespace iroha::subscription

#endif  // IROHA_SUBSCRIPTION_SUBSCRIPTION_ENGINE_HPP
