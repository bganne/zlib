SHELL := /bin/bash
K1_TOOLCHAIN_DIR := $(shell echo $${K1_TOOLCHAIN_DIR:-/usr/local/k1tools})

cflags := -g -Wall -Werror -fopenmp
cflags += -O3
lflags := -g -lz -fopenmp

k1-cflags := -Iinclude
k1-lflags := -Llib

cluster-bin :=

define speed_generic_rules
speed_$(1)-srcs := speed.c
speed_$(1)-cflags := -include calgary_$(1).h
speed_$(1)-deps := calgary_$(1).h lib/libz.a
cluster-bin += speed_$(1)
endef

$(foreach m, $(basename $(notdir $(wildcard LargeCalgaryCorpus/*))), $(eval $(call speed_generic_rules,$(m))))

include $(K1_TOOLCHAIN_DIR)/share/make/Makefile.kalray

calgary_%.h: LargeCalgaryCorpus/%
	echo -n "static const char calgary[] = {" > $@
	hexdump -v -e '/1 "0x%02x,"' $< >> $@
	echo "};" >> $@

lib/libz.a:
	(cd .. && prefix=$$PWD/mppa CC=k1-nodeos-gcc CFLAGS="-O3 -g" ./configure --static --const --solo)
	+make -C .. install

%-sim: $(BIN_DIR)/speed_%
	k1-cluster -- ./$<

%-prof: $(BIN_DIR)/speed_%
	k1-cluster --cycle-based --profile -- ./$<

%-hw: $(BIN_DIR)/speed_%
	k1-jtag-runner --exec-file=Cluster0:./$< -- ./$<
