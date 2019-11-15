/*
  SPDX-License-Identifier: LGPL-3.0

  Copyright (c) 2019 Jevgenijs Protopopovs

  This file is part of Sloked project.

  Sloked is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 3 as published by
  the Free Software Foundation.


  Sloked is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Sloked.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SLOKED_SERVICES_SERVICE_H_
#define SLOKED_SERVICES_SERVICE_H_

#include "sloked/kgr/local/Context.h"
#include "sloked/core/Error.h"
#include "sloked/kgr/Value.h"
#include "sloked/sched/EventLoop.h"
#include <utility>
#include <map>
#include <queue>
#include <list>
#include <functional>
#include <variant>
#include <mutex>
#include <condition_variable>

namespace sloked {

	class SlokedServiceContext : public KgrLocalContext {
	 public:
		using KgrLocalContext::KgrLocalContext;
		void Run() final;
		void SetActivationListener(std::function<void()>) final;
		State GetState() const final;

	 protected:
		class Response {
		 public:
			Response(SlokedServiceContext &, const KgrValue &);
			Response(const Response &);
			Response(Response &&);

			Response &operator=(const Response &);
			Response &operator=(const Response &&);
			void Result(KgrValue &&);
			void Error(KgrValue &&);

		 private:
			std::reference_wrapper<SlokedServiceContext> ctx;
			std::variant<std::reference_wrapper<const KgrValue>, KgrValue> id;
		};

		friend class Response;
		using MethodHandler = std::function<void(const std::string &, const KgrValue &, Response &)>;
	 	using Callback = std::function<void()>;

		void BindMethod(const std::string &, MethodHandler);
		void Defer(std::unique_ptr<SlokedAsyncTask>);

		template <typename T>
		void BindMethod(const std::string &method, void (T::*impl)(const std::string &, const KgrValue &, Response &)) {
			this->BindMethod(method, [this, impl](const std::string &method, const KgrValue &params, Response &rsp) {
				(static_cast<T *>(this)->*impl)(method, params, rsp);
			});
		}

		virtual void InvokeMethod(const std::string &, const KgrValue &, Response &);
		virtual void HandleError(const SlokedError &, Response *);

	 private:
		void SendResponse(const KgrValue &, KgrValue &&);
		void SendError(const KgrValue &, KgrValue &&);

		std::map<std::string, MethodHandler> methods;
		SlokedDefaultEventLoop eventLoop;
	};

	class SlokedServiceClient {
	 public:
	 	class Response {
		 public:
			Response(bool, KgrValue &&);
			bool HasResult() const;
			const KgrValue &GetResult() const;
			const KgrValue &GetError() const;

		 private:
			bool has_result;
			KgrValue content;
		};

		class ResponseHandle {
		 public:
			ResponseHandle(int64_t, SlokedServiceClient &);
			ResponseHandle(const ResponseHandle &) = delete;
			ResponseHandle(ResponseHandle &&);
			~ResponseHandle();

			ResponseHandle &operator=(const ResponseHandle &) = delete;
			ResponseHandle &operator=(ResponseHandle &&);

			void Drop();
			Response Get();
			std::optional<Response> GetOptional();
			bool Has();
			void Notify(std::function<void()>);

		 private:
			int64_t id;
			std::reference_wrapper<SlokedServiceClient> client;
		};
		friend class ResponseHandle;

		SlokedServiceClient(std::unique_ptr<KgrPipe>);
		KgrPipe::Status GetStatus() const;
		ResponseHandle Invoke(const std::string &, KgrValue &&);
		void Close();

	 private:
	 	void SetCallback(int64_t, std::function<void()>);
		void ClearHandle(int64_t);
		bool Has(int64_t);
		Response Get(int64_t);
		void Drop(int64_t);

		std::unique_ptr<std::mutex> mtx;
		std::unique_ptr<std::condition_variable> cv;
		std::unique_ptr<KgrPipe> pipe;
		int64_t nextId;
		std::map<int64_t, std::queue<Response>> pending;
		std::map<int64_t, std::function<void()>> callbacks;
	};
}

#endif