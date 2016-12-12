.arm
.global ioAK2Delay
.type   ioAK2Delay STT_FUNC

@ioAK2Delay ( u32 us )
ioAK2Delay:
	subs r0, #1
	bgt ioAK2Delay
	bx lr
