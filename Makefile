BUILD_DIR = build

CMAKE_OPTIONS += -G "Unix Makefiles"
CMAKE_OPTIONS += -DCMAKE_BUILD_TYPE="Release"

.PHONY: all clean depend
all:

all clean depend: $(BUILD_DIR)/Makefile
	make -C $(dir $<) $(MAKECMDGOALS)

$(BUILD_DIR)/Makefile:
	mkdir -p $(dir $@)
	cd $(dir $@) && cmake $(realpath .) $(CMAKE_OPTIONS)

.PHONY: distclean
distclean:
	rm -rf $(BUILD_DIR)
