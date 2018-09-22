include config.mk
include rules.mk

.PHONY: all

BINARIES =  \
  main
all: $(patsubst %, bin/%, ${BINARIES})

MAIN_DEPS =  \
	ast  \
	operations  \
	parser  \
	pretty  \
	reader  \
	visitable  \
	main
bin/main: $(patsubst %, obj/%.o, ${MAIN_DEPS})

-include ${DEPENDS}
