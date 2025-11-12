CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra
TARGET1 = fastq_merger
TARGET2 = seq_replacer
SOURCES1 = main.c fastq_parser.c id_generator.c file_merger.c utils.c
SOURCES2 = seq_replace_main.c seq_replacer.c fastq_parser.c utils.c
OBJECTS1 = $(SOURCES1:.c=.o)
OBJECTS2 = $(SOURCES2:.c=.o)
HEADERS = fastq_parser.h id_generator.h file_merger.h utils.h seq_replacer.h
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean test install uninstall

all: $(TARGET1) $(TARGET2)

$(TARGET1): main.o fastq_parser.o id_generator.o file_merger.o utils.o
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET2): seq_replace_main.o seq_replacer.o fastq_parser.o utils.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

seq_replace_main.o: seq_replace_main.c seq_replacer.h utils.h
	$(CC) $(CFLAGS) -c $<

seq_replacer.o: seq_replacer.c seq_replacer.h fastq_parser.h utils.h
	$(CC) $(CFLAGS) -c $<

fastq_parser.o: fastq_parser.c fastq_parser.h utils.h
	$(CC) $(CFLAGS) -c $<

id_generator.o: id_generator.c id_generator.h utils.h
	$(CC) $(CFLAGS) -c $<

file_merger.o: file_merger.c file_merger.h fastq_parser.h id_generator.h utils.h
	$(CC) $(CFLAGS) -c $<

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET1) $(TARGET2)

test: $(TARGET1) $(TARGET2)
	@echo "Running tests..."
	@if [ -f run_tests.sh ]; then ./run_tests.sh; else echo "No test script found"; fi

install: $(TARGET1) $(TARGET2)
	@echo "Installing $(TARGET1) and $(TARGET2) to $(BINDIR)..."
	@mkdir -p $(BINDIR)
	@install -m 0755 $(TARGET1) $(BINDIR)
	@install -m 0755 $(TARGET2) $(BINDIR)
	@echo "Installation complete"

uninstall:
	@echo "Uninstalling from $(BINDIR)..."
	@rm -f $(BINDIR)/$(TARGET1)
	@rm -f $(BINDIR)/$(TARGET2)
	@echo "Uninstallation complete"
