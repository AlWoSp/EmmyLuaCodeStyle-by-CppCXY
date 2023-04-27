﻿#include "CodeService/Diagnostic/NameStyle/NameStyleRuleMatcher.h"
#include "Util/StringUtil.h"


bool NameStyleRuleMatcher::Match(LuaSyntaxNode &n, const LuaSyntaxTree &t, const std::vector<NameStyleRule> &rules) {
    if (rules.empty()) {
        return true;
    }
    
    for (auto &rule: rules) {
        auto text = n.GetText(t);
        switch (rule.Type) {
            case NameStyleType::SnakeCase: {
                if (SnakeCase(text)) {
                    return true;
                }
                break;
            }
            case NameStyleType::UpperSnakeCase: {
                if (UpperSnakeCase(text)) {
                    return true;
                }
                break;
            }
            case NameStyleType::CamelCase: {
                if (CamelCase(text)) {
                    return true;
                }
                break;
            }
            case NameStyleType::PascalCase: {
                if (PascalCase(text)) {
                    return true;
                }
                break;
            }
            case NameStyleType::Same: {
                auto data = rule.Data;
                if (data) {
                    auto sameData = std::dynamic_pointer_cast<SameNameStyleData>(data);
                    if (Same(text, sameData->Param)) {
                        return true;
                    }
                }
                break;
            }
            case NameStyleType::Pattern: {
                auto data = rule.Data;
                if (data) {
                    auto patternData = std::dynamic_pointer_cast<PatternNameStyleData>(data);
                    if (PatternMatch(text, patternData)) {
                        return true;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return false;
}

bool NameStyleRuleMatcher::SnakeCase(std::string_view source) {
    enum class ParseState {
        None,
        PrefixUnderscore,
        PrefixUnderscore2,
        Underscore,
        Letter,
        EndDigit
    } state = ParseState::None;

    for (std::size_t index = 0; index != source.size(); index++) {
        char ch = source[index];
        if (ch < 0) {
            return false;
        }

        switch (state) {
            case ParseState::None: {
                if (ch == '_') {
                    state = ParseState::PrefixUnderscore;
                } else if (::islower(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::PrefixUnderscore: {
                if (ch == '_') {
                    state = ParseState::PrefixUnderscore2;
                } else if (::islower(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::PrefixUnderscore2: {
                if (::islower(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::Underscore: {
                if (::islower(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }

                break;
            }
            case ParseState::Letter: {
                if (::islower(ch)) {
                    // ignore
                } else if (ch == '_') {
                    state = ParseState::Underscore;
                } else if (::isdigit(ch)) {
                    state = ParseState::EndDigit;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::EndDigit: {
                if (!::isdigit(ch)) {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

bool NameStyleRuleMatcher::UpperSnakeCase(std::string_view text) {
    enum class ParseState {
        None,
        Underscore,
        Letter,
        EndDigit
    } state = ParseState::None;

    auto source = text;
    for (std::size_t index = 0; index != source.size(); index++) {
        char ch = source[index];
        if (ch < 0) {
            return false;
        }

        switch (state) {
            case ParseState::None: {
                if (::isupper(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::Underscore: {
                if (::isupper(ch)) {
                    state = ParseState::Letter;
                } else {
                    return false;
                }

                break;
            }
            case ParseState::Letter: {
                if (::isupper(ch)) {
                    // ignore
                } else if (ch == '_') {
                    state = ParseState::Underscore;
                } else if (::isdigit(ch)) {
                    state = ParseState::EndDigit;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::EndDigit: {
                if (!::isdigit(ch)) {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

bool NameStyleRuleMatcher::CamelCase(std::string_view text) {
    enum class ParseState {
        None,
        PrefixUnderscore,
        PrefixUnderscore2,
        LowerLetter,
        UpperLetter
    } state = ParseState::None;

    auto source = text;
    for (std::size_t index = 0; index != source.size(); index++) {
        char ch = source[index];
        if (ch < 0) {
            return false;
        }

        switch (state) {
            case ParseState::None: {
                if (ch == '_') {
                    state = ParseState::PrefixUnderscore;
                } else if (::islower(ch)) {
                    state = ParseState::LowerLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::PrefixUnderscore: {
                if (ch == '_') {
                    state = ParseState::PrefixUnderscore2;
                } else if (::islower(ch)) {
                    state = ParseState::LowerLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::PrefixUnderscore2: {
                if (::islower(ch)) {
                    state = ParseState::LowerLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::LowerLetter: {
                if (::islower(ch) || ::isdigit(ch)) {
                    // ignore
                } else if (::isupper(ch)) {
                    state = ParseState::UpperLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::UpperLetter: {
                if (::isupper(ch) || ::isdigit(ch)) {
                    // ignore
                } else if (::islower(ch)) {
                    state = ParseState::LowerLetter;
                } else {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

bool NameStyleRuleMatcher::PascalCase(std::string_view text) {
    enum class ParseState {
        None,
        LowerLetter,
        UpperLetter
    } state = ParseState::None;

    auto source = text;
    for (std::size_t index = 0; index != source.size(); index++) {
        char ch = source[index];
        if (ch < 0) {
            return false;
        }

        switch (state) {
            case ParseState::None: {
                if (::isupper(ch)) {
                    state = ParseState::UpperLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::LowerLetter: {
                if (::islower(ch) || ::isdigit(ch)) {
                    // ignore
                } else if (::isupper(ch)) {
                    state = ParseState::UpperLetter;
                } else {
                    return false;
                }
                break;
            }
            case ParseState::UpperLetter: {
                if (::isupper(ch) || ::isdigit(ch)) {
                    // ignore
                } else if (::islower(ch)) {
                    state = ParseState::LowerLetter;
                } else {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

bool NameStyleRuleMatcher::Same(std::string_view text, std::string_view param) {
    return text == param;
}

bool NameStyleRuleMatcher::PatternMatch(std::string_view text, std::shared_ptr<PatternNameStyleData> data) {
    std::match_results<std::string_view::const_iterator> mc;
    if (!std::regex_match(text.begin(), text.end(), mc, data->Re)) {
        return false;
    }

    for (auto group: data->GroupRules) {
        if (group.GroupId < mc.size()) {
            auto &subMatch = mc[group.GroupId];
            switch (group.Rule) {
                case NameStyleType::Off: {
                    break;
                }
                case NameStyleType::CamelCase: {
                    if (!CamelCase(subMatch.str())) {
                        return false;
                    }
                    break;
                }
                case NameStyleType::PascalCase: {
                    if (!PascalCase(subMatch.str())) {
                        return false;
                    }
                    break;
                }
                case NameStyleType::SnakeCase: {
                    if (!SnakeCase(subMatch.str())) {
                        return false;
                    }
                    break;
                }
                case NameStyleType::UpperSnakeCase: {
                    if (!UpperSnakeCase(subMatch.str())) {
                        return false;
                    }
                    break;
                }
                default: {
                    return false;
                }
            }
        }
    }

    return true;
}
