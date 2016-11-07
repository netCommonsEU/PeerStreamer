cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
              > /dev/null 2>&1; then echo "$(1)"; fi ;)

ld-option = $(shell if echo "int main(){return 0;}" | \
		$(CC) $(LDFLAGS) $(CFLAGS) $(1) -o /dev/null -xc - \
		> /dev/null 2>&1; then echo "$(1)"; fi ;)
