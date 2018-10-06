include config.mk
include rules.mk

.PHONY: all

BINARIES =  \
  gel
all: $(patsubst %, bin/%, ${BINARIES})

GEL_DEPS =  \
	analysis  \
	ast  \
	parser  \
	reader  \
	target-c  \
	visitable  \
	main
bin/gel: $(patsubst %, obj/%.o, ${GEL_DEPS})

-include ${DEPENDS}
