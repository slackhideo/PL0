FILE=report
LATEX=latex
RAIL=rail
DVIPDF=dvipdfm

All:
	$(LATEX) $(FILE).tex
	$(RAIL) -h $(FILE)
	$(LATEX) $(FILE).tex
	$(DVIPDF) $(FILE).dvi

.PHONY: clean
clean:
	rm -f *.aux *.dvi *.log *.rai *.rao
