#pragma once

#include "ast.h"

#include <iostream>

namespace target::c {

void Compile(const ast::TopLevel& top_level, std::ostream* output);

}  // namespace target::c
