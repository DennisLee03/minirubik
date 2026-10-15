.PHONY: all clean rank test-rank

all: rank

rank:
	@gcc -o rank rank.c

test-rank: rank
	@./rank

clean:
	@rm rank