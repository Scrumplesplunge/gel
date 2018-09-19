MAKEFILES = Makefile rules.mk config.mk

.PHONY: default
default: all

# Build with optimized configuration.
.PHONY: opt
opt: all

# Build with debug configuration.
.PHONY: debug
debug: all

.PHONY: clean
clean:
	@echo Removing output directories
	@rm -r {obj,dep,bin}

# Pattern rule for generating a binary.
bin/%: | ${MAKEFILES}
	@echo Linking $*
	@mkdir -p $(dir $@)
	@${CXX} ${LDFLAGS} $^ -o $@ ${LDLIBS}

# Pattern rule for compiling a cc file into an o file.
obj/%.o: src/%.cc $(wildcard src/%.h) | ${MAKEFILES}
	@echo Compiling $*
	@mkdir -p {obj,dep}/$(dir $*)
	@${CXX} ${CXXFLAGS} ${CPPFLAGS} -MMD -MF dep/$*.d $< -c -o obj/$*.o

DEPENDS = $(shell [[ -d dep ]] && find dep -name '*.d')
