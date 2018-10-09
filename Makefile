include config.mk
include rules.mk

.PHONY: all

BINARIES =  \
  gel
all: $(patsubst %, bin/%, ${BINARIES})

GEL_DEPS =  \
	analysis  \
	ast  \
	one_of  \
	parser  \
	reader  \
	target-c  \
	util  \
	value  \
	main
bin/gel: $(patsubst %, obj/%.o, ${GEL_DEPS})

-include ${DEPENDS}
