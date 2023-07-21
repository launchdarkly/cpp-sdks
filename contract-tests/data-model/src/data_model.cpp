#include "data_model/data_model.hpp"

EvaluateFlagParams::EvaluateFlagParams()
    : valueType{ValueType::Unspecified}, detail{false} {}

CommandParams::CommandParams() : command{Command::Unknown} {}
