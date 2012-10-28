; DIRECTIONS:
; Use bootcode.bin from an older firmware release (when loader.bin still existed)
; Assemble this file into "loader.bin" and put on the sd card
; Use the included C# program (or write another one with the same protocol) and send VC4 bytecode.
; NOTE: You need to reset the Pi (unplug it) after each run, apparently due to instruction caching
; (I haven't figured out how to do memory barriers/flushes yet).

; Because of the low system clock rate, this baud rate might be inaccurate
; So be careful with your serial/terminal, some adjustment may be necessary.
.equ TARGET_BAUD_RATE, 115200

; System clock is running directly off the 19.2MHz crystal at initial reset
.equ SYSTEM_CLOCK, 19200000

.equ GPFSEL1, 0x7e200004
.equ GPSET0, 0x7e20001C
.equ GPCLR0, 0x7e200028
.equ GPPUD, 0x7e200094
.equ GPPUDCLK0, 0x7e200098

.equ AUX_ENABLES, 0x7e215004
.equ AUX_MU_IO_REG, 0x7e215040
.equ AUX_MU_IER_REG, 0x7e215044
.equ AUX_MU_IIR_REG, 0x7e215048
.equ AUX_MU_LCR_REG, 0x7e21504C
.equ AUX_MU_MCR_REG, 0x7e215050
.equ AUX_MU_LSR_REG, 0x7e215054
.equ AUX_MU_MSR_REG, 0x7e215058
.equ AUX_MU_SCRATCH, 0x7e21505C
.equ AUX_MU_CNTL_REG, 0x7e215060
.equ AUX_MU_STAT_REG, 0x7e215064
.equ AUX_MU_BAUD_REG, 0x7e215068

.equ MAX_PROG_SIZE, 0x00001000


.space			0x200


; ENTRY POINT
bl	uart_init

mov	r0, 0x87654321
bl	uart_send_int

notmain:
;0x7C indicates start of data
bl	uart_recv_byte
cmp	r0, 0x7C
bne	notmain

;Read # of bytes to receive
; r7 = end addr
; r8 = target addr
bl	uart_recv_int
mov	r7, r0
mov	r8, 0x60008000
add	r7, r8

;Read program bytes
readprogbytes:
bl	uart_recv_byte
stb	r0, (r8)
add	r8, 1
cmp	r8, r7
bne	readprogbytes

;wait some cycles (not sure if this is necessary)
mov	r0, 1000000
DELAY_LOOP_1:
sub	r0, 1
cmp	r0, 0
bgt	DELAY_LOOP_1

;Run program
;r0 = address to write to
mov	r7, 0x60008000
mov	r8, r7
add	r8, MAX_PROG_SIZE
mov	r0, r8
bl	r7

; r0						= # of bytes to be sent
mov	r8, r0
bl	uart_send_int ; Send # of bytes

cmp	r8, 0
beq	notmain

; Send return bytes
; r8 = end addr
; r7 = source addr
mov	r7, 0x60008000
add	r7, MAX_PROG_SIZE
add	r8, r7

sendreturnbytes:
ldb	r0, (r7)
bl	uart_send_byte
add	r7, 1
cmp	r7, r8
bne	sendreturnbytes

b	notmain  ;wait for next program to be sent


; initialize the UART
uart_init:
; Configure TX and RX GPIO pins for Mini Uart function.
mov	r1, GPFSEL1
ld	r0, (r1)
and	r0, ~(7<<12)
or	r0, 2<<12
and	r0, ~(7<<15)
or	r0, 2<<15
st	r0, (r1)

mov	r1, GPPUD
mov	r0, 0
st	r0, (r1)

delay1:
add	r0, 1
cmp	r0, 150
bne	delay1

mov	r1, GPPUDCLK0
mov	r0, (1<<14)|(1<<15)
st	r0, (r1)

mov	r0, 0
delay2:
add	r0, 1
cmp	r0, 150
bne	delay2

mov	r1, GPPUDCLK0
mov	r0, 0
st	r0, (r1)

; Set up serial port
mov	r1, AUX_ENABLES
mov	r0, 1
st	r0, (r1)

mov	r1, AUX_MU_IER_REG
mov	r0, 0
st	r0, (r1)

mov	r1, AUX_MU_CNTL_REG
mov	r0, 0
st	r0, (r1)

mov	r1, AUX_MU_LCR_REG
mov	r0, 3
st	r0, (r1)

mov	r1, AUX_MU_MCR_REG
mov	r0, 0
st	r0, (r1)

mov	r1, AUX_MU_IER_REG
mov	r0, 0
st	r0, (r1)

mov	r1, AUX_MU_IIR_REG
mov	r0, 0xC6
st	r0, (r1)

mov	r1, AUX_MU_BAUD_REG
mov	r0, ((SYSTEM_CLOCK/(TARGET_BAUD_RATE*8))-1)
st	r0, (r1)

mov	r1, AUX_MU_LCR_REG
mov	r0, 3
st	r0, (r1)

mov	r1, AUX_MU_CNTL_REG
mov	r0, 3
st	r0, (r1)

rts ;uart_init


; receive a 4-byte integer (returned in r0)
; assuming it's sent in big-endian format!
uart_recv_int:
push r6, lr
mov	r6, 0

bl	uart_recv_byte
lsl	r0, 24
or	r6, r0

bl	uart_recv_byte
lsl	r0, 16
or	r6, r0

bl	uart_recv_byte
lsl	r0, 8
or	r6, r0

bl	uart_recv_byte
or	r6, r0

mov	r0, r6
pop r6, pc ;uart_recv_int


; send a 4-byte integer (param in r0)
; sent in big-endian format!
uart_send_int:
push r6, lr
mov	r6, r0

lsr	r0, 24
and	r0, 0x000000FF
bl	uart_send_byte

mov	r0, r6
lsr	r0, 16
and	r0, 0x000000FF
bl	uart_send_byte

mov	r0, r6
lsr	r0, 8
and	r0, 0x000000FF
bl	uart_send_byte

mov	r0, r6
and	r0, 0x000000FF
bl	uart_send_byte

pop r6, pc ;uart_send_int


; send 1 byte (param in r0)
uart_send_byte:
mov	r1, AUX_MU_LSR_REG
; loop until space available in Tx buffer
sendwait:
ld	r2, (r1)
and	r2, 0x20
cmp	r2, 0x20
bne	sendwait

mov	r1, AUX_MU_IO_REG
stb	r0, (r1)
rts ;uart_send_byte


; receive 1 byte (returned in r0)
uart_recv_byte:
mov	r1, AUX_MU_LSR_REG
mov	r2, AUX_MU_IO_REG
; loop until char available
recvwait:
ld	r5, (r1)
and	r5, 0x1
cmp	r5, 0x1
bne	recvwait

ldb	r0, (r2)
rts ;uart_recv_byte
