#Check the TARGET
ARG1=$(shell echo $(MAKECMDGOALS) | awk  '{print $$1}')

CFLAGS=-g -Wall -D_GNU_SOURCE

ifeq ($(ARG1), fr2012)
CFLAGS+=-msoft-float
CFLAGS+=-DFIRENZE-2012
endif

help :
	@echo
	@echo "-------------------------------------------------------------"
	@echo "   make common  - building rzscontrol for EchoB/P, X10P, NT558(HW Float)"
	@echo "   make fr2012   - building rzscontrol for Firenze-2012(SW Float)"
	@echo "-------------------------------------------------------------"
	@echo

common: all 
	@echo "---------------------------------------------------------------"
	@echo "   Build Done - building rzscontrol for EchoB/P, X10P, NT558(HW Float)"
	@echo "---------------------------------------------------------------"

fr2012: all
	@echo "--------------------------------------------------------------"
	@echo "   Build Done - building rzscontrol for Firenze-2012(SW Float)"
	@echo "--------------------------------------------------------------"

all:
	$(CROSS_COMPILE)gcc $(CFLAGS) rzscontrol.c -o rzscontrol -I ../include -I../..
	#$(CROSS_COMPILE)-gcc -g -Wall -D_GNU_SOURCE rzscontrol.c -o rzscontrol -I ../include -I../..

doc:
	@xmllint --noout --valid man/rzscontrol.xml
	@xsltproc -o man/ /usr/share/sgml/docbook/xsl-stylesheets/manpages/docbook.xsl \
		man/rzscontrol.xml
	@man2html man/rzscontrol.1 > man/rzscontrol.html

clean:
	@rm -rf rzscontrol
#	@rm -rf man/rzscontrol.1
