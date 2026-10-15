.PHONY: all clean rank test-rank

all: rank specific_mod test-rank test-mod verify test-verify

rank:
	@gcc -o rank rank.c

test-rank: rank
	@./rank

specific_mod:
	@gcc -o specific_mod specific_mod.c

test-mod: specific_mod
	@./specific_mod

verify:
	@gcc -o verify verify.c

test-verify: verify
	@./verify

clean:
	@rm rank specific_mod verify