#pragma once

#include "analysis.h"
#include "ast.h"

#include <iostream>

namespace target::c {

void Compile(const analysis::GlobalContext& context,
             const ast::TopLevel& top_level, std::ostream* output);

}  // namespace target::c
