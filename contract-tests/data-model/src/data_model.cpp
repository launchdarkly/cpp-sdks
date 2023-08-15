#include "data_model/data_model.hpp"

EvaluateFlagParams::EvaluateFlagParams()
    : valueType{ValueType::Unspecified}, detail{false}, context{std::nullopt} {}

CommandParams::CommandParams() : command{Command::Unknown} {}
