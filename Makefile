all:
.SILENT:
.SECONDARY:
PRECMD=echo $@ ; mkdir -p $(@D) ;

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit
LD:=gcc
LDPOST:=

CFILES:=$(wildcard src/*.c)
OFILES:=$(patsubst src/%.c,mid/%.o,$(CFILES))
-include $(OFILES:.o=.d)

EXE:=out/egg2-adulterator
all:$(EXE)

mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<
$(EXE):$(OFILES);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

run:$(EXE);$(EXE) etc/zennoniwa.html etc/image/6-insertable.png

clean:;rm -rf mid out
