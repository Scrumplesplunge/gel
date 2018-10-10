CXX = clang++

CXXFLAGS += -std=c++17 -Werror -Weverything  \
            -Wno-c++98-compat  \
            -Wno-c++98-compat-pedantic  \
            -Wno-padded  \
            -Wno-return-std-move-in-c++11
opt: CXXFLAGS += -ffunction-sections -fdata-sections -flto -Ofast -march=native
debug: CXXFLAGS += -O0 -g

LDFLAGS += -fuse-ld=gold
opt: LDFLAGS += -s -Wl,--gc-sections -flto -Ofast

LDLIBS = -lpthread
