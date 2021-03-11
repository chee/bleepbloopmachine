.DEFAULT_GOAL := send

.PHONY: send
send:
	cp code/code.py /media/chee/prettychips/

.PHONY: init
init:
	cp -r code/* /media/chee/prettychips/
