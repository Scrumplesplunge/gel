#pragma once

#include "analysis.h"
#include "ast.h"

#include <iostream>

namespace target::c {

void Compile(const std::vector<types::Type>& types,
             const analysis::AnnotatedAst::TopLevel& top_level,
             std::ostream* output);

}  // namespace target::c
