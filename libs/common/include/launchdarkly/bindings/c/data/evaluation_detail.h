// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/export.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDEvalDetail* LDEvalDetail;
typedef struct _LDEvalReason* LDEvalReason;

enum LDEvalReason_Kind {
    // The flag was off and therefore returned its configured off value.
    LD_EVALREASON_OFF = 0,
    // The flag was on but the context did not match any targets or rules.
    LD_EVALREASON_FALLTHROUGH = 1,
    // The context key was specifically targeted for this flag.
    LD_EVALREASON_TARGET_MATCH = 2,
    // The context matched one of the flag's rules.
    LD_EVALREASON_RULE_MATCH = 3,
    // The flag was considered off because it had at least one prerequisite
    // flag that either was off or did not return the desired variation.
    LD_EVALREASON_PREREQUISITE_FAILED = 4,
    // The flag could not be evaluated, e.g. because it does not exist or
    // due to an unexpected error.
    LD_EVALREASON_ERROR = 5
};

enum LDEvalReason_ErrorKind {
    // The SDK was not yet fully initialized and cannot evaluate flags.
    LD_EVALREASON_ERROR_CLIENT_NOT_READY = 0,
    // The application did not pass valid context attributes to the SDK
    // evaluation method.
    LD_EVALREASON_ERROR_USER_NOT_SPECIFIED = 1,
    // No flag existed with the specified flag key.
    LD_EVALREASON_ERROR_FLAG_NOT_FOUND = 2,
    // The application requested an evaluation result of one type but the
    // resulting flag variation value was of a different type.
    LD_EVALREASON_ERROR_WRONG_TYPE = 3,
    // The flag had invalid properties.
    LD_EVALREASON_ERROR_MALFORMED_FLAG = 4,
    // An unexpected error happened that stopped evaluation.
    LD_EVALREASON_ERROR_EXCEPTION = 5,
};

/**
 * Frees the detail structure optionally returned by *VariationDetail functions.
 * @param detail Evaluation detail to free.
 */
LD_EXPORT(void)
LDEvalDetail_Free(LDEvalDetail detail);

/**
 * Returns variation index of the evaluation result, if any.
 * @param detail Evaluation detail.
 * @param out_variation_index Pointer where index should be stored, if any.
 * @return True if an index was present, false otherwise.
 */
LD_EXPORT(bool)
LDEvalDetail_VariationIndex(LDEvalDetail detail, size_t* out_variation_index);

/**
 * Returns the reason of the evaluation result, if any.
 * @param detail Evaluation detail.
 * @param out_reason Pointer where reason should be stored, if any. The reason's
 * lifetime is valid only for that of the containing EvalDetail.
 * @return True if a reason was present, false otherwise.
 */
LD_EXPORT(bool)
LDEvalDetail_Reason(LDEvalDetail detail, LDEvalReason* out_reason);

/**
 * Returns the evaluation reason's kind.
 * @param reason Evaluation reason.
 * @return Kind of reason.
 */
LD_EXPORT(enum LDEvalReason_Kind)
LDEvalReason_Kind(LDEvalReason reason);

/**
 * Returns the evaluation reason's error kind, if the evaluation reason's kind
 * was LD_EVALREASON_ERROR.
 * @param reason Evaluation reason.
 * @param out_error_kind Pointer where error kind should be stored, if any.
 * @return True if an error kind was present, false otherwise.
 */
LD_EXPORT(bool)
LDEvalReason_ErrorKind(LDEvalReason reason,
                       enum LDEvalReason_ErrorKind* out_error_kind);

/**
 * Whether the evaluation was part of an experiment.
 *
 * @param reason Evaluation reason.
 * @return True if the evaluation resulted in an experiment rollout and
 * served one of the variations in the experiment.
 */
LD_EXPORT(bool)
LDEvalReason_InExperiment(LDEvalReason reason);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
