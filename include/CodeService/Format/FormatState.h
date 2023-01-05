#pragma once

#include <array>
#include "CodeService/Config/LuaStyle.h"
#include "CodeService/Config/LuaDiagnosticStyle.h"
#include "CodeService/Format/Analyzer/FormatAnalyzer.h"
#include "Types.h"

class FormatState {
public:
    FormatState();

    void SetFormatStyle(LuaStyle &style);

    const LuaStyle &GetStyle() const;

    void SetDiagnosticStyle(LuaDiagnosticStyle &style);

    const LuaDiagnosticStyle &GetDiagnosticStyle() const;

    EndOfLine GetEndOfLine() const;

    bool IsNewLine() const;

    std::size_t &GetCurrentWidth();

    void AddRelativeIndent(LuaSyntaxNode &syntaxNoe, std::size_t indent);

    void AddInvertIndent(LuaSyntaxNode &syntaxNoe, std::size_t indent);

    void RecoverIndent();

    IndentState GetCurrentIndent() const;

    template<class T>
    void AddAnalyzer() {
        _analyzers[static_cast<std::size_t>(T::Type)] = std::make_unique<T>();
    }

    template<class T>
    T *GetAnalyzer() {
        auto &ptr = _analyzers[static_cast<std::size_t>(T::Type)];
        if (ptr) {
            return dynamic_cast<T *>(ptr.get());
        }
        return nullptr;
    }

    void Analyze(const LuaSyntaxTree &t);

    std::array<std::unique_ptr<FormatAnalyzer>, static_cast<std::size_t>(FormatAnalyzerType::Count)>&
    GetAnalyzers();

private:
    LuaStyle _formatStyle;
    LuaDiagnosticStyle _diagnosticStyle;
    EndOfLine _fileEndOfLine;
    std::size_t _currentWidth;
    std::stack<IndentState> _indentStack;

    std::array<std::unique_ptr<FormatAnalyzer>, static_cast<std::size_t>(FormatAnalyzerType::Count)> _analyzers;
};