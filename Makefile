CC	:= pio

.PHONY: all
all:
	$(CC) run

.PHONY: upload
upload:
	$(CC) run --target upload

.PHONY: clean
clean: clean
	$(CC) run --target clean
