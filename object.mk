main = ../../src/main.cpp
deps = ""
.PHONY: all
all:
	@echo -e '\t\t'Making dependencies...
	@../../scripts/deps.sh $(deps) $(CC) $(CFLAGS)
	@echo -e '\t\t'Making source...
	@../../scripts/includes.sh $(main) $(CC) $(CFLAGS)
	@touch *
