root_dir       := ../..
tools_dir      := $(root_dir)/tools
user_dir       := $(root_dir)/user
INCLUDES       := -I$(root_dir)/include

.PRECIOUS: %.b %.b.c

%.x: %.b.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	$(tools_dir)/bintoc -f $< -o $@ -p test

%.b: entry.o %.o
	$(LD) -o $@ $(LDFLAGS) -T $(user_dir)/user.lds $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: all clean

all: hash1.x hash2.x hash3.x

clean:
	rm -rf *.o *~ *.x *.b *.b.c
