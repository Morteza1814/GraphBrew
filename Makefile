# =========================================================
# Compiler and Directories Setup
# =========================================================
CXX = g++
# =========================================================
BENCH_DIR = bench
BIN_DIR = $(BENCH_DIR)/bin
LIB_DIR = $(BENCH_DIR)/lib
SRC_DIR = $(BENCH_DIR)/src
INC_DIR = $(BENCH_DIR)/include
# =========================================================
INCLUDE_GAP    = $(INC_DIR)/gapbs 
INCLUDE_RABBIT = $(INC_DIR)/rabbit
INCLUDE_GORDER = $(INC_DIR)/gorder
# =========================================================
DEP_GAP    = $(wildcard $(INCLUDE_GAP)/*.h) 
DEP_RABBIT = $(wildcard $(INCLUDE_RABBIT)/*.hpp)
DEP_GORDER = $(wildcard $(INCLUDE_GORDER)/*.hpp) $(wildcard $(INCLUDE_GORDER)/*.cpp) 
# =========================================================

# =========================================================
#     CLI COMMANDS                           
# =========================================================
COMPILED_FILE = $(@F)
CREATE        = [${BLUE}create!${NC}]
SUCCESS       = [${GREEN}success!${NC}]
FAIL          = [${RED}failure!${NC}]
CREATE_MSG   = echo  "$(CREATE) $(COMPILED_FILE)"
SUCCESS_MSG   = echo  "$(SUCCESS) $(COMPILED_FILE)"
FAIL_MSG      = echo  "$(FAIL) $(COMPILED_FILE)"
EXIT_STATUS   = &&  $(SUCCESS_MSG) || { $(FAIL_MSG) ; exit 1; }
CREATE_STATUS  = &&  $(CREATE_MSG) || { $(FAIL_MSG) ; exit 1; }
# TEST PASS OR FAIL
PASS = \033[92mPASS\033[0m
FAIL = \033[91mFAIL\033[0m
# =========================================================
# Color coded messages                      
# =========================================================
YELLOW  =\033[0;33m
GREEN   =\033[0;32m
BLUE    =\033[0;34m
RED     =\033[0;31m
NC      =\033[0m
# =========================================================

# =========================================================
# Compiler Flags
# =========================================================
CXXFLAGS_GAP    = -std=c++17 -O3 -Wall -fopenmp
CXXFLAGS_RABBIT = -mcx16 
CXXFLAGS_GORDER = -m64 -mcpu=native 
# =========================================================
LDLIBS_RABBIT   += -ltcmalloc_minimal -lnuma
# =========================================================
CXXFLAGS = $(CXXFLAGS_GAP) $(CXXFLAGS_RABBIT) 
LDLIBS   = $(LDLIBS_RABBIT)
# =========================================================
# CXXFLAGS += -D_DEBUG
INCLUDES = -I$(INCLUDE_GAP) -I$(INCLUDE_RABBIT) 
# =========================================================

# =========================================================
# Runtime Flags OMP_NUM_THREADS
# =========================================================
PARALLEL = 4
# GRAPH_BENCH = /test/graphs/graph.el
GRAPH_BENCH = -g 5
# =========================================================
# Targets
# =========================================================
KERNELS = bc bfs cc cc_sv pr pr_spmv sssp tc
SUITE = $(addprefix $(BIN_DIR)/,$(KERNELS)) $(BIN_DIR)/converter
# =========================================================



.PHONY: all run-% help-% help clean run-%-gdb run-%-sweep
all: $(SUITE)

# =========================================================
# Compilation Rules
# =========================================================
$(BIN_DIR)/%: $(SRC_DIR)/%.cc $(DEP_GAP) $(DEP_RABBIT) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LDLIBS) -o $@ $(EXIT_STATUS)

# =========================================================
# Running Benchmarks
# =========================================================
RUN_PARAMS = $(GRAPH_BENCH) -n 1 -o 3 -v
# RUN_PARAMS = -s -g 6 -n 1 -o 1

run-%: $(BIN_DIR)/%
	@OMP_NUM_THREADS=$(PARALLEL) ./$< $(RUN_PARAMS) $(EXIT_STATUS)

run-%-gdb: $(BIN_DIR)/%
	@OMP_NUM_THREADS=1 gdb -ex=r --args ./$< $(RUN_PARAMS)

run-%-mem: $(BIN_DIR)/%
	@OMP_NUM_THREADS=1 valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./$< $(RUN_PARAMS)

run-all: $(addprefix run-, $(KERNELS))

# Define a rule that sweeps through -o 1 to 7
run-%-sweep: $(BIN_DIR)/%
	@for o in 1 8; do \
		echo "Running with -o $$o"; \
		OMP_NUM_THREADS=$(PARALLEL) ./$(BIN_DIR)/$* -v $(GRAPH_BENCH) -n 1 -o $$o; \
	done
# =========================================================
# Directory Setup
# =========================================================
$(BIN_DIR):
	@mkdir -p $@ $(CREATE_STATUS)

# =========================================================
# Cleanup
# =========================================================
clean:
	@rm -rf $(BIN_DIR) $(EXIT_STATUS)

# =========================================================
# Help
# =========================================================
help: help-pr
	@echo "Available Make commands:"
	@echo "  all            - Builds all targets including GAP benchmarks (CPU)"
	@echo "  run-%          - Runs the specified GAP benchmark (bc bfs cc cc_sv pr pr_spmv sssp tc)"
	@echo "  help-%         - Print the specified Help (bc bfs cc cc_sv pr pr_spmv sssp tc)"
	@echo "  clean          - Removes all build artifacts"
	@echo "  help           - Displays this help message"

help-%: $(BIN_DIR)/%
	@./$< -h 
	@echo ""
	@echo "Reordering Algorithms:"
	@echo "  - ORIGINAL      (0): No reordering applied."
	@echo "  - RANDOM        (1): Apply random reordering."
	@echo "  - SORT          (2): Apply sort-based reordering."
	@echo "  - HUBSORT       (3): Apply hub-based sorting."
	@echo "  - HUBCLUSTER    (4): Apply clustering based on hub scores."
	@echo "  - DBG           (5): Apply degree-based grouping."
	@echo "  - HUBSORTDBG    (6): Combine hub sorting with degree-based grouping."
	@echo "  - HUBCLUSTERDBG (7): Combine hub clustering with degree-based grouping."
	@echo "  - RABBITORDER   (8): Apply community clustering with incremental aggregation."
	@echo "  - GORDER        (9): Apply dynamic programming BFS and windowing ordering."
	@echo "  - MAP           (10): Requires a file format for reordering. Use the -r 10:filename.label option."
	@echo ""
	@echo "Example Usage:"
	@echo "  make all - Compile the program."
	@echo "  make clean - Clean build files."
	@echo "  ./$< -g 15 -n 1 -r 10:mapping.label - Execute with MAP reordering using 'mapping.label'."

help-all: $(addprefix help-, $(KERNELS))