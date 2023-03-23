#include "attribute_reference.hpp"
#include <tuple>

namespace launchdarkly {

enum class ParseState {
    kBegin,        /* start state */
    kPlain,        /* plain, top-level attribute name detected */
    kTokenBegin,   /* found the beginning of a token */
    kSearchingEnd, /* searching for the end of a token */
    kEscapeBegin,  /* found start of an escape sequence */
    kRefEnd        /* end state */
};

enum class ParseEvent {
    kNoop,         /* no event */
    kChar,         /* write the input character */
    kTilde,        /* write a '~' */
    kForwardSlash, /* write a '/' */
    kTokenEnd,     /* end of token */
    kInputEnd,     /* end of input */
    kError         /* error */
};

/**
 * This function is responsible for decoding an input string,
 * representing a reference, into a list of components.
 *
 * Each component is the bits between the '/' separators. For example,
 * the reference "/foo/bar" will be represented by the components "foo" and
 * "bar".
 *
 * The algorithm proceeds as a single-pass over the input string, performed by
 * a state machine.
 */
std::tuple<ParseState, ParseEvent> ParseChar(ParseState state, char input) {
    switch (state) {
        case ParseState::kBegin: {
            switch (input) {
                case '/': {
                    return {ParseState::kTokenBegin, ParseEvent::kNoop};
                }
                case '\0': {
                    return {ParseState::kRefEnd, ParseEvent::kError};
                }
                default: {
                    return {ParseState::kPlain, ParseEvent::kChar};
                }
            }
        }
        case ParseState::kPlain: {
            if (input == '\0') {
                return {ParseState::kRefEnd, ParseEvent::kInputEnd};
            }

            return {ParseState::kPlain, ParseEvent::kChar};
        }
        case ParseState::kTokenBegin: {
            switch (input) {
                case '\0':  // Falling through to the error case intentionally.
                case '/': {
                    return {ParseState::kRefEnd, ParseEvent::kError};
                }
                case '~': {
                    return {ParseState::kEscapeBegin, ParseEvent::kNoop};
                }
                default: {
                    return {ParseState::kSearchingEnd, ParseEvent::kChar};
                }
            }
        }
        case ParseState::kSearchingEnd: {
            switch (input) {
                case '\0': {
                    return {ParseState::kRefEnd, ParseEvent::kInputEnd};
                }
                case '~': {
                    return {ParseState::kEscapeBegin, ParseEvent::kNoop};
                }
                case '/': {
                    return {ParseState::kTokenBegin, ParseEvent::kTokenEnd};
                }
                default: {
                    return {ParseState::kSearchingEnd, ParseEvent::kChar};
                }
            }
        }
        case ParseState::kEscapeBegin: {
            switch (input) {
                case '0': {
                    return {ParseState::kSearchingEnd, ParseEvent::kTilde};
                }
                case '1': {
                    return {ParseState::kSearchingEnd,
                            ParseEvent::kForwardSlash};
                }
                default: {
                    return {ParseState::kRefEnd, ParseEvent::kError};
                }
            }
        }
        case ParseState::kRefEnd: {
            return {ParseState::kRefEnd, ParseEvent::kNoop};
        }
    }
    // Should only happen if additional states are added but not handled.
    return {ParseState::kRefEnd, ParseEvent::kError};
}

bool ParseRef(std::string str, std::vector<std::string>& components) {
    auto p_state = ParseState::kBegin;

    std::string tmp_token;
    // The loop here extends to the size of the string, so we can send a null
    // into the parsing logic to terminate the parsing.
    for (auto index = 0; index <= str.size(); index++) {
        // The character in the string, or null if we go out of bounds of the
        // string.
        char c = index < str.size() ? str[index] : '\0';
        auto [new_p_state, event] = ParseChar(p_state, c);
        p_state = new_p_state;

        switch (event) {
            case ParseEvent::kNoop:
                continue;
            case ParseEvent::kChar: {
                tmp_token.push_back(c);
            } break;
            case ParseEvent::kForwardSlash: {
                tmp_token.push_back('/');
            } break;
            case ParseEvent::kTilde: {
                tmp_token.push_back('~');
            } break;
            case ParseEvent::kTokenEnd: {
                components.push_back(std::move(tmp_token));
                // Could .clear here, but this seems more appropriate.
                tmp_token = std::string();
            } break;
            case ParseEvent::kInputEnd:
                components.push_back(std::move(tmp_token));
                return true;
            case ParseEvent::kError:
                return false;
        }
    }
    return false;
}

/**
 * Literal starting with a '/' needs to be converted to an attribute
 * reference string.
 */
std::string EscapeLiteral(std::string const& literal) {
    std::string escaped = "/";
    for (auto& c : literal) {
        if (c == '~') {
            escaped.append("~0");
        } else if (c == '/') {
            escaped.append("~1");
        } else {
            escaped.push_back(c);
        }
    }
    return escaped;
}

AttributeReference::AttributeReference(std::string str, bool literal) {
    if (literal) {
        components_.push_back(str);
        // Literal starting with a '/' needs to be converted to an attribute
        // reference string.
        if (str[0] == '/') {
            redaction_name_ = EscapeLiteral(str);
        } else {
            redaction_name_ = str;
        }
    } else {
        valid_ = ParseRef(str, components_);
        if (valid_) {
            redaction_name_ = std::move(str);
        } else {
            components_.clear();
        }
    }
}

AttributeReference AttributeReference::from_literal_str(std::string lit_str) {
    return {std::move(lit_str), true};
}

AttributeReference AttributeReference::from_reference_str(std::string ref_str) {
    return {std::move(ref_str), false};
}

std::string const& AttributeReference::component(size_t depth) const {
    if (depth < components_.size()) {
        return components_[depth];
    }
    return empty_;
}

size_t AttributeReference::depth() const {
    return components_.size();
}

bool AttributeReference::is_kind() const {
    return depth() == 1 && component(0) == "kind";
}

bool AttributeReference::valid() const {
    return valid_;
}

std::string const& AttributeReference::redaction_name() const {
    return redaction_name_;
}

}  // namespace launchdarkly