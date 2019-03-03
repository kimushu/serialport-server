BUILD_DIR = build

ifneq ($(OS),Windows_NT)
CMAKE_OPTIONS += -DCMAKE_BUILD_TYPE=Release
endif

.PHONY: all clean depend
all:

ifneq ($(OS),Windows_NT)
all clean depend: $(BUILD_DIR)/Makefile
	make -C $(dir $<) $(MAKECMDGOALS)
else
all: $(BUILD_DIR)/Makefile
	cmake --build $(BUILD_DIR) --target ALL_BUILD --config Release
endif

$(BUILD_DIR)/Makefile:
	mkdir -p $(dir $@)
	cd $(dir $@) && cmake $(realpath .) $(CMAKE_OPTIONS)

.PHONY: distclean
distclean:
	rm -rf $(BUILD_DIR)

.PHONY: doc
doc:
	doxygen Doxyfile
