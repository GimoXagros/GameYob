ROOT := ../..

.PHONY: all clean gameyob.nds gameyob_dsi.nds

all gameyob.nds gameyob_dsi.nds:
	$(MAKE) -C $(ROOT) -f Makefile.blocksds $@

clean:
	$(MAKE) -C $(ROOT) -f Makefile.blocksds clean
