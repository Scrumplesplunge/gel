include config.mk
include rules.mk

.PHONY: all

BINARIES =  \
  gel
all: $(patsubst %, bin/%, ${BINARIES})

GEL_DEPS =  \
	ast  \
	parser  \
	reader  \
	target-c  \
	visitable  \
	main
bin/gel: $(patsubst %, obj/%.o, ${GEL_DEPS})

-include ${DEPENDS}
