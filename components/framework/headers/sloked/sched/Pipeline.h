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

#ifndef SLOKED_SCHED_PIPELINE_H_
#define SLOKED_SCHED_PIPELINE_H_

#include <tuple>
#include <vector>

#include "sloked/core/Meta.h"
#include "sloked/sched/Task.h"

namespace sloked {

    template <typename Source, typename E = void>
    struct UnwrapTaskResult {
        using Type = Source;
        static auto Run(Source &&task, std::weak_ptr<SlokedLifetime> lifetime) {
            return std::move(task);
        }
    };

    template <typename Source>
    struct UnwrapTaskResult<Source,
                            std::enable_if_t<IsInstantiation<
                                TaskResult, typename Source::Result>::value>> {
        using Type = typename UnwrapTaskResult<typename Source::Result>::Type;
        static_assert(
            std::is_same_v<typename Source::Error, typename Type::Error>);
        static auto Run(Source &&task, std::weak_ptr<SlokedLifetime> lifetime) {
            typename Type::Supplier supplier;
            task.Notify(
                [supplier, lifetime](const auto &unwrapped) {
                    switch (unwrapped.State()) {
                        case TaskResultStatus::Ready:
                            UnwrapTaskResult<decltype(
                                unwrapped.GetResult())>::Run(unwrapped
                                                                 .GetResult(),
                                                             lifetime)
                                .Notify([supplier](const auto &result) {
                                    switch (result.State()) {
                                        case TaskResultStatus::Ready:
                                            if constexpr (std::is_void_v<
                                                              typename Type::
                                                                  Result>) {
                                                supplier.SetResult();
                                            } else {
                                                supplier.SetResult(std::move(
                                                    result.GetResult()));
                                            }
                                            break;

                                        case TaskResultStatus::Error:
                                            supplier.SetError(
                                                std::move(result.GetError()));
                                            break;

                                        case TaskResultStatus::Cancelled:
                                            supplier.Cancel();
                                            break;

                                        default:
                                            break;
                                    }
                                });
                            break;

                        case TaskResultStatus::Error:
                            supplier.SetError(unwrapped.GetError());
                            break;

                        case TaskResultStatus::Cancelled:
                            supplier.Cancel();
                            break;

                        default:
                            break;
                    }
                },
                lifetime);
            return supplier.Result();
        }
    };

    template <typename... Stage>
    struct SlokedTaskPipelineApplier;

    template <typename CurrentStage, typename... OtherStages>
    struct SlokedTaskPipelineApplier<CurrentStage, OtherStages...> {
        template <typename Source>
        static auto Apply(Source &&task, std::weak_ptr<SlokedLifetime> lifetime,
                          CurrentStage &&stage, OtherStages &&... otherStages) {
            return SlokedTaskPipelineApplier<OtherStages...>::Apply(
                stage(std::forward<Source>(task), lifetime), lifetime,
                std::forward<OtherStages>(otherStages)...);
        }

        template <typename State, typename Source>
        static auto Apply(State &&state, Source &&task,
                          std::weak_ptr<SlokedLifetime> lifetime,
                          CurrentStage &&stage, OtherStages &&... otherStages) {
            return SlokedTaskPipelineApplier<OtherStages...>::Apply(
                std::forward<State>(state),
                stage(std::forward<State>(state), std::forward<Source>(task),
                      lifetime),
                lifetime, std::forward<OtherStages>(otherStages)...);
        }
    };

    template <typename CurrentStage>
    struct SlokedTaskPipelineApplier<CurrentStage> {
        template <typename Source>
        static auto Apply(Source &&task, std::weak_ptr<SlokedLifetime> lifetime,
                          CurrentStage &&stage) {
            return stage(std::forward<Source>(task), lifetime);
        }

        template <typename State, typename Source>
        static auto Apply(State &&state, Source &&task,
                          std::weak_ptr<SlokedLifetime> lifetime,
                          CurrentStage &&stage) {
            return stage(std::forward<State>(state), std::forward<Source>(task),
                         lifetime);
        }
    };

    template <typename... Stage>
    class SlokedTaskPipeline {
        using Applier = SlokedTaskPipelineApplier<Stage...>;

     public:
        SlokedTaskPipeline(Stage... stages)
            : stages{std::make_tuple(std::move(stages)...)} {}

        template <typename Source>
        auto operator()(Source &&task,
                        std::weak_ptr<SlokedLifetime> lifetime) const {
            auto applier = &Applier::template Apply<Source>;
            return std::apply(
                applier,
                std::tuple_cat(
                    std::make_tuple(std::forward<Source>(task), lifetime),
                    this->stages));
        }

        template <typename State, typename Source>
        auto operator()(State &&state, Source &&task,
                        std::weak_ptr<SlokedLifetime> lifetime) const {
            auto applier = &Applier::template Apply<State, Source>;
            return std::apply(
                applier,
                std::tuple_cat(
                    std::make_tuple(std::forward<State>(state),
                                    std::forward<Source>(task), lifetime),
                    this->stages));
        }

     private:
        std::tuple<Stage...> stages;
    };

    namespace SlokedTaskPipelineStages {

        class Cancel : public std::exception {};
        template <typename Stage>
        class Async {
         public:
            Async(Stage stage) : stage(std::move(stage)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->stage, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(
                    BindFirst(this->stage, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename StageFn, typename Source>
            static auto Invoke(const StageFn &stage, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                using Target = decltype(stage(
                    std::declval<typename UnwrapTaskResult<Source>::Type>(),
                    lifetime));
                typename Target::Supplier supplier;
                UnwrapTaskResult<Source>::Run(std::forward<Source>(src),
                                              lifetime)
                    .Notify(
                        [stage, supplier, lifetime](const auto &result) {
                            auto stageResult = stage(result, lifetime);
                            stageResult.Notify(
                                [supplier](const auto &stageResult) {
                                    Notify<decltype(stageResult),
                                           decltype(supplier), Target>(
                                        stageResult, supplier);
                                },
                                lifetime);
                        },
                        lifetime);
                return supplier.Result();
            }

            template <typename Result, typename Supplier, typename Target>
            static void Notify(const Result &stageResult,
                               const Supplier &supplier) {
                switch (stageResult.State()) {
                    case TaskResultStatus::Ready:
                        if constexpr (std::is_void_v<typename Target::Result>) {
                            supplier.SetResult();
                        } else {
                            supplier.SetResult(
                                std::move(stageResult.GetResult()));
                        }
                        break;

                    case TaskResultStatus::Error:
                        supplier.SetError(std::move(stageResult.GetError()));
                        break;

                    case TaskResultStatus::Cancelled:
                        supplier.Cancel();
                        break;

                    default:
                        break;
                }
            }
            Stage stage;
        };

        template <typename F>
        class Map {
            template <typename Arg, typename Fn, typename E = void>
            struct InvokeResult;

            template <typename Arg, typename Fn>
            struct InvokeResult<Arg, Fn,
                                std::enable_if_t<!std::is_void_v<
                                    std::remove_reference_t<Arg>>>> {
                using Type = std::invoke_result_t<Fn, Arg>;
            };

            template <typename Fn>
            struct InvokeResult<void, Fn, void> {
                using Type = std::invoke_result_t<Fn>;
            };

            template <typename TransformFn, typename Arg, typename E = void>
            struct Transform;

            template <typename TransformFn, typename Arg>
            struct Transform<
                TransformFn, Arg,
                std::enable_if_t<!std::is_void_v<typename Arg::Result>>> {
                static auto Make(const TransformFn &transform, const Arg &src) {
                    using Result = typename InvokeResult<typename Arg::Result,
                                                         TransformFn>::Type;
                    if constexpr (std::is_void_v<Result>) {
                        transform(src.GetResult());
                    } else {
                        return transform(src.GetResult());
                    }
                }
            };

            template <typename TransformFn, typename Arg>
            struct Transform<
                TransformFn, Arg,
                std::enable_if_t<std::is_void_v<typename Arg::Result>>> {
                static auto Make(const TransformFn &transform, const Arg &src) {
                    using Result = typename InvokeResult<typename Arg::Result,
                                                         TransformFn>::Type;
                    if constexpr (std::is_void_v<Result>) {
                        transform();
                    } else {
                        return transform();
                    }
                }
            };

         public:
            Map(F transform) : transform(std::move(transform)) {}

            template <typename Source>
            auto operator()(const Source &src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return this->Invoke(this->transform, src, std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return this->Invoke(
                    BindFirst(this->transform, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename TransformFn, typename Source>
            static auto Invoke(const TransformFn &transform, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                using SrcResult =
                    typename std::remove_reference_t<Source>::Result;
                using TargetResult =
                    typename InvokeResult<SrcResult, TransformFn>::Type;
                using Error = typename std::remove_reference_t<Source>::Error;
                TaskResultSupplier<TargetResult, Error> supplier;
                src.Notify(
                    [transform, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                try {
                                    if constexpr (std::is_void_v<
                                                      TargetResult>) {
                                        Transform<
                                            TransformFn,
                                            std::remove_reference_t<decltype(
                                                result)>>::Make(transform,
                                                                result);
                                        supplier.SetResult();
                                    } else {
                                        supplier.SetResult(
                                            Transform<TransformFn,
                                                      std::remove_reference_t<
                                                          decltype(result)>>::
                                                Make(transform, result));
                                    }
                                } catch (const Cancel &) {
                                    supplier.Cancel();
                                } catch (const Error &err) {
                                    supplier.SetError(err);
                                }
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(std::move(result.GetError()));
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    lifetime);
                return supplier.Result();
            }
            F transform;
        };

        template <typename F>
        class MapError {
         public:
            MapError(F transform) : transform(std::move(transform)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return this->Invoke(this->transform, std::forward<Source>(src),
                                    std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return this->Invoke(
                    BindFirst(this->transform, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename TransformFn, typename Source>
            static auto Invoke(const TransformFn &transform, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                using Result = typename std::remove_reference_t<Source>::Result;
                using SrcError =
                    typename std::remove_reference_t<Source>::Error;
                using TargetError = std::invoke_result_t<F, SrcError>;
                TaskResultSupplier<Result, TargetError> supplier;
                src.Notify(
                    [transform, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                if constexpr (std::is_void_v<Result>) {
                                    supplier.SetResult();
                                } else {
                                    supplier.SetResult(
                                        std::move(result.GetResult()));
                                }
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(transform(result.GetError()));
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    lifetime);
                return supplier.Result();
            }
            F transform;
        };
        template <typename F>
        class MapCancelled {
         public:
            MapCancelled(F generate) : generate(std::move(generate)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->generate, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(
                    BindFirst(this->generate, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename GenerateFn, typename Source>
            static auto Invoke(const GenerateFn &generate, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                using Result = typename std::remove_reference_t<Source>::Result;
                using Error = typename std::remove_reference_t<Source>::Error;
                TaskResultSupplier<Result, Error> supplier;
                src.Notify(
                    [generate, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                if constexpr (std::is_void_v<Result>) {
                                    supplier.SetResult();
                                } else {
                                    supplier.SetResult(
                                        std::move(result.GetResult()));
                                }
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(std::move(result.GetError()));
                                break;

                            case TaskResultStatus::Cancelled:
                                try {
                                    if constexpr (std::is_void_v<Result>) {
                                        generate();
                                        supplier.SetResult();
                                    } else {
                                        supplier.SetResult(generate());
                                    }
                                } catch (const Error &err) {
                                    supplier.SetError(err);
                                }
                                break;

                            default:
                                break;
                        }
                    },
                    lifetime);
                return supplier.Result();
            }
            F generate;
        };

        template <typename F>
        class Scan {
         public:
            Scan(F scanner) : scanner(std::move(scanner)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->scanner, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return this->Invoke(
                    BindFirst(this->scanner, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename ScannerFn, typename Source>
            static auto Invoke(const ScannerFn &scanner, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                src.Notify(
                    [scanner](const auto &value) {
                        if (value.State() == TaskResultStatus::Ready) {
                            if constexpr (std::is_void_v<
                                              typename std::remove_reference_t<
                                                  Source>::Result>) {
                                scanner();
                            } else {
                                scanner(value.GetResult());
                            }
                        }
                    },
                    lifetime);
                return Source{std::move(src)};
            }
            F scanner;
        };

        template <typename F>
        class ScanErrors {
         public:
            ScanErrors(F scanner) : scanner(std::move(scanner)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->scanner, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(
                    BindFirst(this->scanner, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename ScannerFn, typename Source>
            static auto Invoke(const ScannerFn &scanner, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                src.Notify(
                    [scanner](const auto &value) {
                        if (value.State() == TaskResultStatus::Error) {
                            scanner(value.GetError());
                        }
                    },
                    lifetime);
                return Source{std::move(src)};
            }
            F scanner;
        };

        template <typename F>
        class ScanCancelled {
         public:
            ScanCancelled(F scanner) : scanner(std::move(scanner)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->scanner, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(
                    BindFirst(this->scanner, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename ScannerFn, typename Source>
            static auto Invoke(const ScannerFn &scanner, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                src.Notify(
                    [scanner](const auto &value) {
                        if (value.State() == TaskResultStatus::Cancelled) {
                            scanner(value);
                        }
                    },
                    lifetime);
                return Source{std::move(src)};
            }
            F scanner;
        };

        template <typename F>
        class Catch {
         public:
            Catch(F catcher) : catcher(std::move(catcher)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(this->catcher, std::forward<Source>(src),
                              std::move(lifetime));
            }

            template <typename State, typename Source>
            auto operator()(State &&state, Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                return Invoke(
                    BindFirst(this->catcher, std::forward<State>(state)),
                    std::forward<Source>(src), std::move(lifetime));
            }

         private:
            template <typename CatcherFn, typename Source>
            static auto Invoke(const CatcherFn &catcher, Source &&src,
                               std::weak_ptr<SlokedLifetime> lifetime) {
                using Result = typename std::remove_reference_t<Source>::Result;
                using Error = typename std::remove_reference_t<Source>::Error;
                TaskResultSupplier<Result, Error> supplier;
                src.Notify(
                    [catcher, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                if constexpr (std::is_void_v<Result>) {
                                    supplier.SetResult();
                                } else {
                                    supplier.SetResult(
                                        std::move(result.GetResult()));
                                }
                                break;

                            case TaskResultStatus::Error:
                                try {
                                    supplier.SetResult(
                                        catcher(result.GetError()));
                                } catch (const Error &err) {
                                    supplier.SetError(err);
                                }
                                break;

                            case TaskResultStatus::Cancelled:
                                supplier.Cancel();
                                break;

                            default:
                                break;
                        }
                    },
                    lifetime);
                return supplier.Result();
            }
            F catcher;
        };

        template <typename F>
        class Finally {
         public:
            Finally(F scanner) : scanner(std::move(scanner)) {}

            template <typename Source>
            Source operator()(Source &&src,
                              std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(this->scanner, std::move(lifetime));
                return src;
            }

            template <typename State, typename Source>
            Source operator()(State &&state, Source &&src,
                              std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(BindFirst(this->scanner, std::forward<State>(state)),
                           std::move(lifetime));
                return src;
            }

         private:
            F scanner;
        };

        class Pass {
         public:
            template <typename Source>
            Source operator()(Source &&src,
                              std::weak_ptr<SlokedLifetime> lifetime) const {
                return src;
            }
        };
    }  // namespace SlokedTaskPipelineStages

    template <typename... Stage>
    class SlokedAsyncTaskPipeline {
        using BasePipeline =
            SlokedTaskPipeline<SlokedTaskPipelineStages::Async<Stage>...>;

     public:
        SlokedAsyncTaskPipeline(Stage... stages)
            : base{SlokedTaskPipelineStages::Async<Stage>{
                  std::move(stages)}...} {}

        template <typename Source>
        auto operator()(Source &&src,
                        std::weak_ptr<SlokedLifetime> lifetime) const {
            auto result = this->base(std::forward<Source>(src), lifetime);
            return UnwrapTaskResult<decltype(result)>::Run(std::move(result),
                                                           std::move(lifetime));
        }

        template <typename State, typename Source>
        auto operator()(State &&state, Source &&src,
                        std::weak_ptr<SlokedLifetime> lifetime) const {
            auto result = this->base(std::forward<State>(state),
                                     std::forward<Source>(src), lifetime);
            return UnwrapTaskResult<decltype(result)>::Run(std::move(result),
                                                           std::move(lifetime));
        }

     private:
        BasePipeline base;
    };
}  // namespace sloked

#endif