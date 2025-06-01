CC=gcc
CFLAGS=-O3 -march=native -funroll-loops -ffast-math -DNDEBUG -Wall -Wextra
LDFLAGS=-lpthread -lm

# Target for building the shared library
kulami_game.so: main.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< $(LDFLAGS)

# Target for building a regular executable (for testing)
kulami_game: main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean target
clean:
	rm -f kulami_game.so kulami_game

# Install target (optional)
install: kulami_game.so
	cp kulami_game.so /usr/local/lib/
	ldconfig

.PHONY: clean install
