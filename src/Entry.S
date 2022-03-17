	.section	.stack, "aw", %nobits
.globl gStack
.type  gStack, @object
gStack:
	.skip 512*1024
.size	gStack, .-gStack
.globl gStackEnd
gStackEnd:

	.section	.text.head
.globl _start
.type  _start, @function
_start:
	la sp, gStackEnd
	li fp, 0
	call Do
	j .
.size	_start, .-_start
