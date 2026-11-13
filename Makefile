OUTPUT := bin/program
LIBRARY := libscl/libscl.a
MKFLAGS := --no-print-directory

all: $(OUTPUT)

$(OUTPUT): $(LIBRARY)
	make $(MKFLAGS) -C tests rebuild -j4 --output-sync

$(LIBRARY):
	make $(MKFLAGS) -C libscl rebuild -j4 --output-sync


