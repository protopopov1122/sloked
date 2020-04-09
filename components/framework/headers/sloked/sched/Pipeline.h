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
    };

    template <typename CurrentStage>
    struct SlokedTaskPipelineApplier<CurrentStage> {
        template <typename Source>
        static auto Apply(Source &&task, std::weak_ptr<SlokedLifetime> lifetime,
                          CurrentStage &&stage) {
            return stage(std::forward<Source>(task), lifetime);
        }
    };

    template <typename... Stage>
    class SlokedTaskPipeline {
        using Applier = SlokedTaskPipelineApplier<Stage...>;

     public:
        SlokedTaskPipeline(Stage... stages)
            : stages{std::make_tuple(std::move(stages)...)} {}

        template <typename Source>
        auto operator()(Source &&task, std::weak_ptr<SlokedLifetime> lifetime =
                                           SlokedLifetime::Global) const {
            auto applier = &Applier::template Apply<Source>;
            return std::apply(
                applier,
                std::tuple_cat(
                    std::make_tuple(std::forward<Source>(task), lifetime),
                    this->stages));
        }

     private:
        std::tuple<Stage...> stages;
    };

    namespace SlokedTaskPipelineStages {
        template <typename Stage>
        class Async {
         public:
            Async(Stage stage) : stage(std::move(stage)) {}

            template <typename Source>
            auto operator()(Source &&src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                using Target =
                    decltype(this->stage(std::forward<Source>(src), lifetime));
                typename Target::Supplier supplier;
                src.Notify(
                    [this, supplier, lifetime](const auto &result) {
                        auto stageResult = this->stage(result, lifetime);
                        stageResult.Notify(
                            [supplier](const auto &stageResult) {
                                switch (stageResult.State()) {
                                    case TaskResultStatus::Ready:
                                        supplier.SetResult(
                                            std::move(stageResult.GetResult()));
                                        break;

                                    case TaskResultStatus::Error:
                                        supplier.SetError(
                                            std::move(stageResult.GetError()));
                                        break;

                                    case TaskResultStatus::Cancelled:
                                        supplier.Cancel();
                                        break;

                                    default:
                                        break;
                                }
                            },
                            lifetime);
                    },
                    lifetime);
                return supplier.Result();
            }

         private:
            Stage stage;
        };

        template <typename F>
        class Map {
         public:
            Map(F transform) : transform(std::move(transform)) {}

            template <typename Source>
            auto operator()(const Source &src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                using SrcResult = typename Source::Result;
                using TargetResult = std::invoke_result_t<F, SrcResult>;
                using Error = typename Source::Error;
                TaskResultSupplier<TargetResult, Error> supplier;
                src.Notify(
                    [this, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                try {
                                    supplier.SetResult(
                                        this->transform(result.GetResult()));
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

         private:
            F transform;
        };

        template <typename F>
        class MapError {
         public:
            MapError(F transform) : transform(std::move(transform)) {}

            template <typename Source>
            auto operator()(const Source &src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                using Result = typename Source::Result;
                using SrcError = typename Source::Error;
                using TargetError = std::invoke_result_t<F, SrcError>;
                TaskResultSupplier<Result, TargetError> supplier;
                src.Notify(
                    [this, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult(
                                    std::move(result.GetResult()));
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(
                                    this->transform(result.GetError()));
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

         private:
            F transform;
        };
        template <typename F>
        class MapCancelled {
         public:
            MapCancelled(F generate) : generate(std::move(generate)) {}

            template <typename Source>
            auto operator()(const Source &src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                using Result = typename Source::Result;
                using Error = typename Source::Error;
                TaskResultSupplier<Result, Error> supplier;
                src.Notify(
                    [this, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult(
                                    std::move(result.GetResult()));
                                break;

                            case TaskResultStatus::Error:
                                supplier.SetError(std::move(result.GetError()));
                                break;

                            case TaskResultStatus::Cancelled:
                                try {
                                    supplier.SetResult(this->generate());
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

         private:
            F generate;
        };

        template <typename F>
        class Scan {
         public:
            Scan(F scanner) : scanner(std::move(scanner)) {}

            template <typename T>
            T operator()(const T &src,
                         std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(
                    [this](const auto &value) {
                        if (value.State() == TaskResultStatus::Ready) {
                            this->scanner(value.GetResult());
                        }
                    },
                    lifetime);
                return src;
            }

         private:
            F scanner;
        };

        template <typename F>
        class ScanErrors {
         public:
            ScanErrors(F scanner) : scanner(std::move(scanner)) {}

            template <typename T>
            T operator()(const T &src,
                         std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(
                    [this](const auto &value) {
                        if (value.State() == TaskResultStatus::Error) {
                            this->scanner(value.GetError());
                        }
                    },
                    lifetime);
                return src;
            }

         private:
            F scanner;
        };

        template <typename F>
        class ScanCancelled {
         public:
            ScanCancelled(F scanner) : scanner(std::move(scanner)) {}

            template <typename T>
            T operator()(const T &src,
                         std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(
                    [this](const auto &value) {
                        if (value.State() == TaskResultStatus::Cancelled) {
                            this->scanner(value);
                        }
                    },
                    lifetime);
                return src;
            }

         private:
            F scanner;
        };

        template <typename F>
        class Catch {
         public:
            Catch(F catcher) : catcher(std::move(catcher)) {}

            template <typename Source>
            auto operator()(const Source &src,
                            std::weak_ptr<SlokedLifetime> lifetime) const {
                using Result = typename Source::Result;
                using Error = typename Source::Error;
                TaskResultSupplier<Result, Error> supplier;
                src.Notify(
                    [this, supplier](const auto &result) {
                        switch (result.State()) {
                            case TaskResultStatus::Ready:
                                supplier.SetResult(
                                    std::move(result.GetResult()));
                                break;

                            case TaskResultStatus::Error:
                                try {
                                    supplier.SetResult(
                                        this->catcher(result.GetError()));
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

         private:
            F catcher;
        };

        template <typename F>
        class Finally {
         public:
            Finally(F scanner) : scanner(std::move(scanner)) {}

            template <typename T>
            T operator()(const T &src,
                         std::weak_ptr<SlokedLifetime> lifetime) const {
                src.Notify(this->scanner, lifetime);
                return src;
            }

         private:
            F scanner;
        };

        class Pass {
         public:
            template <typename T>
            T operator()(const T &src,
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
        auto operator()(Source &&src, std::weak_ptr<SlokedLifetime> lifetime =
                                          SlokedLifetime::Global) const {
            return this->base(std::forward<Source>(src), std::move(lifetime));
        }

     private:
        BasePipeline base;
    };
}  // namespace sloked

#endif