/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019-2020 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as
  published by the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SCHED_COMPOUNDTASK_H_
#define SLOKED_SCHED_COMPOUNDTASK_H_

#include <cassert>

#include "sloked/core/Meta.h"
#include "sloked/sched/Pipeline.h"
#include "sloked/sched/Task.h"

namespace sloked {

    class SlokedCompoundTask {
        template <typename... T>
        using AlignedStorageTuple =
            std::tuple<std::aligned_storage_t<sizeof(T), alignof(T)>...>;

        template <typename Storage, typename Result, typename Int, Int... Ints>
        static auto UnwrapAlignedStorageTupleImpl(
            const std::integer_sequence<Int, Ints...> &seq, Storage &storage,
            const Result *) {
            return std::make_tuple(
                std::move(*reinterpret_cast<std::remove_reference_t<decltype(
                              std::get<Ints>(std::declval<Result>()))> *>(
                    &std::get<Ints>(storage)))...);
        }

        template <typename... T>
        static auto UnwrapAlignedStorageTuple(
            VoidSafe_t<AlignedStorageTuple, T...> &storage) {
            return UnwrapAlignedStorageTupleImpl(
                std::index_sequence_for<T...>(), storage,
                static_cast<VoidSafe_t<std::tuple, T...> *>(nullptr));
        }

        template <typename... Task>
        struct StaticCompoundStorage {
            using ResultType = VoidSafe_t<std::tuple, typename Task::Result...>;
            using ErrorType = UniqueVariant_t<typename Task::Error...>;
            using Supplier = TaskResultSupplier<ResultType, ErrorType>;

            Supplier supplier;
            VoidSafe_t<AlignedStorageTuple, typename Task::Result...> results;
            ErrorType errors;
            std::atomic<std::size_t> pending{sizeof...(Task)};
            std::atomic_bool rejected{0};
            std::atomic_bool cancelled{false};

            auto Unwrap() {
                return UnwrapAlignedStorageTuple<typename Task::Result...>(
                    this->results);
            }
        };

        template <std::size_t Index, typename Storage, typename... Task>
        struct NotifyAll;

        template <std::size_t Index, typename Storage>
        struct NotifyAll<Index, Storage> {
            static void Apply(const std::shared_ptr<Storage> &storage,
                              std::weak_ptr<SlokedLifetime> lifetime) {}
        };

        template <std::size_t Index, typename Storage, typename Task,
                  typename... OtherTasks>
        struct NotifyAll<Index, Storage, Task, OtherTasks...> {
            static void Apply(const std::shared_ptr<Storage> &storage,
                              std::weak_ptr<SlokedLifetime> lifetime,
                              Task &&task, OtherTasks &&... other) {
                using ResultType = typename Task::Result;
                task.Notify(
                    [storage](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                if constexpr (std::is_void_v<ResultType>) {
                                    new (&std::get<Index>(storage->results))
                                        VoidType();
                                } else if constexpr (
                                    std::is_move_constructible_v<ResultType>) {
                                    new (&std::get<Index>(storage->results))
                                        ResultType(
                                            std::move(result.GetResult()));
                                } else {
                                    new (&std::get<Index>(storage->results))
                                        ResultType(result.GetResult());
                                }
                                if (--storage->pending == 0) {
                                    std::atomic_thread_fence(
                                        std::memory_order_consume);
                                    if (storage->rejected.load() == 0 &&
                                        !storage->cancelled.load()) {
                                        storage->supplier.SetResult(
                                            storage->Unwrap());
                                    }
                                }
                                break;

                            case TaskResultStatus::Error:
                                if (!storage->rejected.exchange(true)) {
                                    storage->errors =
                                        std::move(result.GetError());
                                    storage->supplier.SetError(
                                        std::move(storage->errors));
                                }
                                std::atomic_thread_fence(
                                    std::memory_order_release);
                                storage->pending--;
                                break;

                            case TaskResultStatus::Cancelled:
                                storage->cancelled.store(true);
                                storage->supplier.Cancel();
                                std::atomic_thread_fence(
                                    std::memory_order_release);
                                storage->pending--;
                                break;

                            default:
                                assert(false);
                                break;
                        }
                    },
                    lifetime);
                NotifyAll<Index + 1, Storage, OtherTasks...>::Apply(
                    storage, std::move(lifetime),
                    std::forward<OtherTasks>(other)...);
            }
        };

        template <typename T>
        struct DynamicCompoundStorage {
            using SafeType = typename SafeVoidWrap<typename T::Result>::Type;
            using Result = std::vector<SafeType>;
            using Error = typename T::Error;
            using Supplier = TaskResultSupplier<Result, Error>;

            DynamicCompoundStorage(std::size_t sz) {
                this->result.reserve(sz);
            }

            Supplier supplier;
            Result result;
        };

        template <typename Iter>
        using DynamicTaskResultType =
            std::remove_reference_t<decltype(*std::declval<Iter>())>;

        template <typename Iter>
        static void IterateTaskResults(
            const Iter &current, const Iter &end,
            const std::shared_ptr<
                DynamicCompoundStorage<DynamicTaskResultType<Iter>>> &storage) {
            if (current != end) {
                auto taskResult = *current;
                taskResult.Notify([next = std::next(current), end,
                                   storage](const auto &result) {
                    switch (result.State()) {
                        case TaskResultStatus::Ready:
                            if constexpr (std::is_void_v<
                                              typename DynamicTaskResultType<
                                                  Iter>::Result>) {
                                storage->result.emplace_back(VoidType{});
                            } else {
                                storage->result.emplace_back(
                                    std::move(result.GetResult()));
                            }
                            if (next != end) {
                                IterateTaskResults(next, end, storage);
                            } else {
                                storage->supplier.SetResult(
                                    std::move(storage->result));
                            }
                            break;

                        case TaskResultStatus::Error:
                            storage->result.clear();
                            storage->supplier.SetError(
                                std::move(result.GetError()));
                            break;

                        case TaskResultStatus::Cancelled:
                            storage->result.clear();
                            storage->supplier.Cancel();
                            break;

                        default:
                            assert(false);
                            break;
                    }
                });
            }
        }

     public:
        template <typename... T>
        static auto All(std::weak_ptr<SlokedLifetime> lifetime, T &&... tasks) {
            auto storage = std::make_shared<StaticCompoundStorage<T...>>();
            NotifyAll<0, StaticCompoundStorage<T...>, T...>::Apply(
                storage, std::move(lifetime), std::forward<T>(tasks)...);
            return storage->supplier.Result();
        }

        template <typename Iter>
        static auto All(const Iter &begin, const Iter &end) {
            auto storage = std::make_shared<
                DynamicCompoundStorage<DynamicTaskResultType<Iter>>>(
                std::distance(begin, end));
            IterateTaskResults(begin, end, storage);
            return storage->supplier.Result();
        }
    };

    class SlokedTaskTransformations {
     public:
        template <typename R, typename E, typename TransformFn>
        static auto Transform(const TaskResult<R, E> &result,
                              TransformFn transform) {
            static const SlokedAsyncTaskPipeline Pipeline(
                SlokedTaskPipelineStages::Map(
                    [](const TransformFn &transform, const R &result) {
                        return transform(result);
                    }));
            return Pipeline(std::move(transform), result,
                            SlokedLifetime::Global);
        }

        template <typename R, typename E>
        static auto Voidify(const TaskResult<R, E> &result) {
            return Transform(result, [](const R &) {});
        }
    };
}  // namespace sloked

#endif