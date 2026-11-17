#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>
#include <launchdarkly/server_side/hooks/hook.hpp>
#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::hooks;

namespace {

/* Serves one flag, which has the single string variation "a". */
class OneFlagReader final : public integrations::ISerializedDataReader {
   public:
    [[nodiscard]] GetResult Get(integrations::ISerializedItemKind const& kind,
                                std::string const& itemKey) const override {
        if (kind.Namespace() != "features" || itemKey != kFlagKey) {
            return std::nullopt;
        }
        return integrations::SerializedItemDescriptor::Present(kFlagVersion,
                                                               kFlag);
    }

    [[nodiscard]] AllResult All(
        integrations::ISerializedItemKind const& kind) const override {
        AllResult::value_type out;
        if (kind.Namespace() == "features") {
            out.emplace(kFlagKey, integrations::SerializedItemDescriptor::Present(
                                      kFlagVersion, kFlag));
        }
        return out;
    }

    [[nodiscard]] std::string const& Identity() const override {
        return identity_;
    }

    [[nodiscard]] bool Initialized() const override { return true; }

    static constexpr char const* kFlagKey = "string-flag";
    static constexpr int kFlagVersion = 5;

   private:
    std::string const kFlag =
        R"({"key":"string-flag","version":5,"on":true,"variations":["a"],)"
        R"("fallthrough":{"variation":0},"salt":"salt"})";
    std::string const identity_ = "one flag reader";
};

/* Records the evaluation detail that the afterEvaluation stage gets. The
 * analytics event for an evaluation is made from this same detail. */
class DetailRecordingHook final : public Hook {
   public:
    HookMetadata const& Metadata() const override { return metadata_; }

    EvaluationSeriesData AfterEvaluation(
        EvaluationSeriesContext const& series_context,
        EvaluationSeriesData data,
        EvaluationDetail<Value> const& detail) override {
        detail_ = detail;
        return data;
    }

    std::optional<EvaluationDetail<Value>> detail_;

   private:
    HookMetadata metadata_{"DetailRecordingHook"};
};

class WrongTypeEvaluationTest : public ::testing::Test {
   protected:
    WrongTypeEvaluationTest()
        : hook_(std::make_shared<DetailRecordingHook>()),
          context_(ContextBuilder().Kind("user", "user-key").Build()) {
        auto builder = ConfigBuilder("sdk-key");
        builder.Events().Disable();
        builder.DataSystem().Method(
            server_side::config::builders::LazyLoadBuilder().Source(
                std::make_shared<OneFlagReader>()));
        builder.Hooks(hook_);
        client_.emplace(std::move(*builder.Build()));
        client_->StartAsync().wait_for(std::chrono::seconds(5));
    }

    /* Checks that the evaluation result, as the hook and the event get it, is
     * the default value with no variation and a WRONG_TYPE reason. */
    void ExpectWrongTypeResult(Value const& default_value) {
        ASSERT_TRUE(hook_->detail_.has_value());
        auto const& detail = *hook_->detail_;
        EXPECT_EQ(detail.Value(), default_value);
        EXPECT_FALSE(detail.VariationIndex().has_value());
        ASSERT_TRUE(detail.Reason().has_value());
        EXPECT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kError);
        EXPECT_EQ(detail.Reason()->ErrorKind(),
                  EvaluationReason::ErrorKind::kWrongType);
    }

    std::shared_ptr<DetailRecordingHook> hook_;
    Context context_;
    std::optional<Client> client_;
};

}  // namespace

TEST_F(WrongTypeEvaluationTest, BoolVariationGivesDefaultToEvent) {
    EXPECT_TRUE(client_->BoolVariation(context_, OneFlagReader::kFlagKey, true));
    ExpectWrongTypeResult(Value(true));
}

TEST_F(WrongTypeEvaluationTest, BoolVariationDetailGivesDefaultToEvent) {
    auto detail =
        client_->BoolVariationDetail(context_, OneFlagReader::kFlagKey, true);
    EXPECT_TRUE(detail.Value());
    EXPECT_EQ(detail.Reason()->ErrorKind(),
              EvaluationReason::ErrorKind::kWrongType);
    ExpectWrongTypeResult(Value(true));
}

TEST_F(WrongTypeEvaluationTest, IntVariationGivesDefaultToEvent) {
    EXPECT_EQ(client_->IntVariation(context_, OneFlagReader::kFlagKey, 3), 3);
    ExpectWrongTypeResult(Value(3));
}

TEST_F(WrongTypeEvaluationTest, DoubleVariationDetailGivesDefaultToEvent) {
    auto detail =
        client_->DoubleVariationDetail(context_, OneFlagReader::kFlagKey, 2.5);
    EXPECT_EQ(detail.Value(), 2.5);
    ExpectWrongTypeResult(Value(2.5));
}

TEST_F(WrongTypeEvaluationTest, StringVariationOfTheCorrectTypeIsNotChanged) {
    EXPECT_EQ(
        client_->StringVariation(context_, OneFlagReader::kFlagKey, "default"),
        "a");
    ASSERT_TRUE(hook_->detail_.has_value());
    EXPECT_EQ(hook_->detail_->Value(), Value("a"));
    EXPECT_EQ(hook_->detail_->VariationIndex(), 0);
}

TEST_F(WrongTypeEvaluationTest, JsonVariationDoesNotCheckTheType) {
    EXPECT_EQ(client_->JsonVariation(context_, OneFlagReader::kFlagKey,
                                     Value(true)),
              Value("a"));
    ASSERT_TRUE(hook_->detail_.has_value());
    EXPECT_EQ(hook_->detail_->VariationIndex(), 0);
}
