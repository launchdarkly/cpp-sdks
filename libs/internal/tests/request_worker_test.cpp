#include "launchdarkly/events/detail/request_worker.hpp"
#include <gtest/gtest.h>
#include <launchdarkly/network/http_requester.hpp>

using namespace launchdarkly::events;
using namespace launchdarkly::network;

struct TestCase {
    State state;
    HttpResult result;
    State expected_state;
    Action expected_action;
};

class StateMachineFixture : public ::testing::TestWithParam<TestCase> {};

TEST_P(StateMachineFixture, StatesAndActionsAreComputedCorrectly) {
    auto params = GetParam();
    auto [next_state, action] = NextState(params.state, params.result);
    ASSERT_EQ(params.expected_state, next_state);
    ASSERT_EQ(params.expected_action, action);
}

INSTANTIATE_TEST_SUITE_P(
    SuccessfulRequests,
    StateMachineFixture,
    ::testing::Values(
        TestCase{State::FirstChance,
                 HttpResult(200, "200 ok!", HttpResult::HeadersType{}),
                 State::Idle, Action::ParseDateAndReset},
        TestCase{State::FirstChance,
                 HttpResult(201, "201 created!", HttpResult::HeadersType{}),
                 State::Idle, Action::ParseDateAndReset},
        TestCase{State::FirstChance,
                 HttpResult(202, "202 accepted!", HttpResult::HeadersType{}),
                 State::Idle, Action::ParseDateAndReset}));

INSTANTIATE_TEST_SUITE_P(
    RecoverableErrors,
    StateMachineFixture,
    ::testing::Values(
        TestCase{State::FirstChance,
                 HttpResult(400, "400 bad request!", HttpResult::HeadersType{}),
                 State::SecondChance, Action::Retry},
        TestCase{
            State::FirstChance,
            HttpResult(408, "408 request timeout!", HttpResult::HeadersType{}),
            State::SecondChance, Action::Retry},
        TestCase{State::FirstChance,
                 HttpResult(429,
                            "429 too many requests!",
                            HttpResult::HeadersType{}),
                 State::SecondChance, Action::Retry},
        TestCase{State::FirstChance,
                 HttpResult(500,
                            "500 internal server error!",
                            HttpResult::HeadersType{}),
                 State::SecondChance, Action::Retry},
        TestCase{State::FirstChance, HttpResult("generic connection error!"),
                 State::SecondChance, Action::Retry},
        TestCase{State::FirstChance,
                 HttpResult(413, "413 too large!", HttpResult::HeadersType{}),
                 State::Idle, Action::Reset}));

INSTANTIATE_TEST_SUITE_P(
    PermanentErrors,
    StateMachineFixture,
    ::testing::Values(
        TestCase{State::FirstChance,
                 HttpResult(404, "404 not found!", HttpResult::HeadersType{}),
                 State::PermanentlyFailed, Action::NotifyPermanentFailure},
        TestCase{
            State::FirstChance,
            HttpResult(418, "418 i'm a teapot!", HttpResult::HeadersType{}),
            State::PermanentlyFailed, Action::NotifyPermanentFailure},
        TestCase{
            State::FirstChance,
            HttpResult(401, "401 unauthorized!", HttpResult::HeadersType{}),
            State::PermanentlyFailed, Action::NotifyPermanentFailure}));

INSTANTIATE_TEST_SUITE_P(
    SecondChanceOutcomes,
    StateMachineFixture,
    ::testing::Values(
        TestCase{State::SecondChance,
                 HttpResult(404, "404 not found!", HttpResult::HeadersType{}),
                 State::PermanentlyFailed, Action::NotifyPermanentFailure},
        TestCase{State::SecondChance,
                 HttpResult(400, "400 bad request!", HttpResult::HeadersType{}),
                 State::Idle, Action::Reset},
        TestCase{
            State::SecondChance,
            HttpResult(408, "408 request timeout!", HttpResult::HeadersType{}),
            State::Idle, Action::Reset},
        TestCase{State::SecondChance,
                 HttpResult(429,
                            "429 too many requests!",
                            HttpResult::HeadersType{}),
                 State::Idle, Action::Reset},
        TestCase{State::SecondChance,
                 HttpResult(200, "200 ok!", HttpResult::HeadersType{}),
                 State::Idle, Action::ParseDateAndReset}));
