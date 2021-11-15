#pragma once

enum class LuaDiagnosisType
{
	DiagnosisElement,
	MaxLineLengthDiagnosis,
	IndentDiagnosis,
	LineLayoutDiagnosis,
	SpaceDiagnosis,
	EqSymbolAlignDiagnosis,
	// δʵ��
	NameStyleDiagnosis,
	KeepTrailingEmptyLine,
};