/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ORG_GENIVI_COMMONAPI_GMOCK_H_
#define ORG_GENIVI_COMMONAPI_GMOCK_H_

#include <gmock/gmock.h>

#include <utility>


#define MOCK_METHOD0_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD0(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m() { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture()); \
		return promise.get_future(); \
	}

#define MOCK_METHOD1_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD1(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1))); \
		return promise.get_future(); \
	}

#define MOCK_METHOD2_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD2(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1, \
            GMOCK_ARG_(, F, 2) gmock_a2) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1), \
				std::forward<GMOCK_ARG_(, F, 2)>(gmock_a2))); \
		return promise.get_future(); \
	}

#define MOCK_METHOD3_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD3(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1, \
            GMOCK_ARG_(, F, 2) gmock_a2, \
            GMOCK_ARG_(, F, 3) gmock_a3) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1), \
				std::forward<GMOCK_ARG_(, F, 2)>(gmock_a2), \
				std::forward<GMOCK_ARG_(, F, 3)>(gmock_a3))); \
		return promise.get_future(); \
	}

#define MOCK_METHOD4_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD4(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1, \
            GMOCK_ARG_(, F, 2) gmock_a2, \
            GMOCK_ARG_(, F, 2) gmock_a3, \
            GMOCK_ARG_(, F, 3) gmock_a4) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1), \
				std::forward<GMOCK_ARG_(, F, 2)>(gmock_a2), \
				std::forward<GMOCK_ARG_(, F, 3)>(gmock_a3), \
				std::forward<GMOCK_ARG_(, F, 4>(gmock_a4))); \
		return promise.get_future(); \
	}

#define MOCK_METHOD5_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD5(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1, \
            GMOCK_ARG_(, F, 2) gmock_a2, \
            GMOCK_ARG_(, F, 2) gmock_a3, \
            GMOCK_ARG_(, F, 2) gmock_a4, \
            GMOCK_ARG_(, F, 3) gmock_a5) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1), \
				std::forward<GMOCK_ARG_(, F, 2)>(gmock_a2), \
				std::forward<GMOCK_ARG_(, F, 3)>(gmock_a3), \
				std::forward<GMOCK_ARG_(, F, 4)>(gmock_a4), \
				std::forward<GMOCK_ARG_(, F, 5)>(gmock_a5))); \
		return promise.get_future(); \
	}

#define MOCK_METHOD6_WITH_FUTURE_RETURN(m, F) \
	MOCK_METHOD6(m##WithoutStdFuture, F); \
	std::future<GMOCK_RESULT_(, F)> m( \
			GMOCK_ARG_(, F, 1) gmock_a1, \
            GMOCK_ARG_(, F, 2) gmock_a2, \
            GMOCK_ARG_(, F, 2) gmock_a3, \
            GMOCK_ARG_(, F, 2) gmock_a4, \
            GMOCK_ARG_(, F, 2) gmock_a5, \
            GMOCK_ARG_(, F, 3) gmock_a6) { \
		std::promise<CommonAPI::CallStatus> promise; \
		promise.set_value(m##WithoutStdFuture( \
				std::forward<GMOCK_ARG_(, F, 1)>(gmock_a1), \
				std::forward<GMOCK_ARG_(, F, 2)>(gmock_a2), \
				std::forward<GMOCK_ARG_(, F, 3)>(gmock_a3), \
				std::forward<GMOCK_ARG_(, F, 4)>(gmock_a4), \
				std::forward<GMOCK_ARG_(, F, 5)>(gmock_a5), \
				std::forward<GMOCK_ARG_(, F, 6)>(gmock_a6))); \
		return promise.get_future(); \
	}

#endif // ORG_GENIVI_COMMONAPI_GMOCK_H_
