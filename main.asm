.include "tn13def.inc"

; r0/1: bootloader

.def	myaddr	= r2
.def	rxaddr	= r3
.def	rxbyte	= r4
.def	t0last	= r5
.def	t0cnt	= r6
.def	sym0_lo	= r7
.def	sym1_lo	= r8
.def	sym1_hi	= r9

.def	temp	= r16
.def	temp2	= r17
.def	ovf	= r18
.def	state	= r19
.def	dimval	= r20
.def	rxcnt	= r21
.def 	cksum	= r22
.def	flags	= r23

; X: 26/27
; Y: 28/29
; Z: 30/31

.equ	FL_DIMVAL	= 0
.equ	FL_TSTDISABLE	= 1
.equ	FL_TSTMODE	= 2

    ; RESET
    rjmp RESET
    ; INT0
    rjmp INT_0
    ; PCINT0
    reti
    ; TIM0_OVF
    rjmp T0_OVF
    ; EE_RDY
    reti
    ; ANA_COMP
    reti
    ; TIM0_COMPA
    reti
    ; TIM0_COMPB
    reti
    ; WATCHDG
    reti
    ; ADC
    reti

T0_OVF:
    cpi ovf, $ff
    breq T0_OVF_OUT
    
    inc ovf
    
T0_OVF_OUT:
    reti

TEST_ENABLE:
    sbr flags, (1<<FL_TSTMODE)
    ; TODO: enable LED
    rjmp RX_RESET
    
TEST_DISABLE:
    cbr flags, (1<<FL_TSTMODE)
    ; TODO: back to PWM mode
    rjmp RX_RESET

DIM_OFF:
    ; disable PWM
    clr temp
    out TCCR0A, temp

    ; off, force dim=0
    ldi temp, (1<<PB2)|(0<<PB0)
    out PORTB, temp
    
    rjmp PKT_START
    
DIM_FULL:
    ; disable PWM
    clr temp
    out TCCR0A, temp

    ; on, force dim=1
    ldi temp, (0<<PB2)|(1<<PB0)
    out PORTB, temp

    rjmp PKT_START

LONG_DELTA:

    sbis PINB, PB1
    rjmp RX_ERROR	; ERROR: too long hi

    cpi rxcnt, 3
    brlo PKT_ERROR	; ERROR: pkt too short

    cpi cksum, 0
    brne PKT_ERROR	; ERROR: cksum invalid

    sbrs flags, FL_DIMVAL	; no new dimval for my addr
    rjmp PKT_START
    
    sbrc flags, FL_TSTMODE	; skip dimval if test mode enabled
    rjmp PKT_START
    
    cpi dimval, 0	; powerdown
    breq DIM_OFF
    
    cpi dimval, 255	; poweron, no pwm
    breq DIM_FULL

    dec dimval		; set dimval - 1
    out OCR0A, dimval
    
    ; fire up PWM if disabled
    ldi temp, (1<<COM0A1)|(1<<WGM01)|(1<<WGM00)
    in temp2, TCCR0A
    sbrs temp2, COM0A1
    out TCCR0A, temp
    
    ; on if not yet done
    cbi PORTB, PB2
    
    rjmp PKT_START
    
PKT_ERROR:

    ; TODO
    ;sbrc flags, FL_TSTMODE	; in test mode: disable LED on rx error
    ;sbi PORTB, PB2
    
PKT_START:
    ; beginning of a new packet
    ldi state, 1
    clr rxcnt
    cbr flags, (1<<FL_DIMVAL)
    
    rjmp INT_0_RET

INT_0:
    in t0cnt, TCNT0
    
    ; calc delta
    
    ; ovf < 1 - no overflow
    cpi ovf, 1
    brlo NO_OVF
    
    ; test mode enable
    cpi ovf, 200
    brsh TEST_ENABLE
    
    ; test mode disable
    cpi ovf, 100
    brsh TEST_DISABLE
    
    ; ovf > 1 - too long
    cpi ovf, 2
    brsh LONG_DELTA
    
    ; 1 overflow
    ; dt = $ff - t0last + t0cnt + 1
    ser temp
    sub temp, t0last
    add temp, t0cnt
    brcs LONG_DELTA
    inc temp
    cpi temp, 0
    breq LONG_DELTA
    
    rjmp DELTA_DONE
    
NO_OVF:

    ; no overflow
    ; dt = t0cnt - t0last
    mov temp, t0cnt
    sub temp, t0last
    
DELTA_DONE:

    cpi temp, 45
    brsh LONG_DELTA

    ; still waiting for silence before startbit
    cpi state, 0
    breq RX_ERROR	; ERROR: silence before start too short

    ; receiving databit?
    cpi state, 2
    brsh DATABIT
    
    ; state = 1 -> receiving startbit
    cpi temp, 21
    brlo RX_ERROR	; ERROR: startbit too short
    
    ; calc etu
    ; sym0 lower bound = 1u - 0.25u
    ; sym1 lower bound = sym0 upper bound = 1u + 0.25u
    ; sym1 upper bound = 2u - 0.25u

    mov sym1_hi, temp	; 1_hi = 2u
    lsr temp		; t = 1u
    mov sym1_lo, temp	; 1_hi = 1u
    mov sym0_lo, temp	; 0_lo = 1u
    lsr temp		; t = 0.5u
    lsr temp		; t = 0.25u
    sub sym0_lo, temp	; 0_lo = 1u - 0.25u
    add sym1_lo, temp	; 1_lo = 1u + 0.25u
    sub sym1_hi, temp	; 1_hi = 2u - 0.25u
    inc sym1_hi		; for brsh
    
    ldi state, 2
    
    rjmp INT_0_RET
    
DATABIT:

    ; < lower -> error
    cp temp, sym0_lo
    brlo RX_ERROR		; ERROR: databit too short
    
    ; > upper -> error
    cp temp, sym1_hi
    brsh RX_ERROR		; ERROR: databit too long
    
    lsl rxbyte
    
    cp temp, sym1_lo	; sym0 -> don't add 1
    brlo DATABIT_DONE
    
    inc rxbyte
    
DATABIT_DONE:

    inc state

    ; new rxbyte complete?
    cpi state, 10
    brne INT_0_RET
    
    ldi state, 2	; next state: new databyte
    
    inc rxcnt		; received++
    
    add cksum, rxbyte	; cksum
    
    cpi rxcnt, 1	; 1st byte is cksum
    breq CKSUM_RCVD
    
    cpi rxcnt, 2	; 2nd byte is addr.
    breq ADDR_RCVD
    
    cp rxaddr, myaddr	; my addr.?
    breq RCV_DIMVAL
    
    ; not for me - inc rxaddr
    inc rxaddr
    rjmp INT_0_RET
    
RCV_DIMVAL:			; save dimval - set later if cksum ok
    mov dimval, rxbyte
    sbr flags, (1<<FL_DIMVAL)
    rjmp INT_0_RET

CKSUM_RCVD:
    mov cksum, rxbyte
    rjmp INT_0_RET

ADDR_RCVD:
    mov rxaddr, rxbyte
    rjmp INT_0_RET
    
RX_ERROR:

RX_RESET:

    clr state
    cbr flags, (1<<FL_DIMVAL)
    
    ; TODO
;    sbrc flags, FL_TSTMODE	; in test mode: disable LED on rx error
;    sbi PORTB, PB2
    
INT_0_RET:
    mov t0last, t0cnt
    clr ovf
    reti

; PB1 = INT0 = PCINT1 = rxbyte in
; timer0 auf 8MHz -> ovf 31.250 kHz
; idee: 30Hz * 8 bit * 16 LEDs * 3 (start/stop) = 11.520 kHz
; *2 = 23.040 kHz
    
RESET:
    ldi temp, low(RAMEND)
    out SPL, temp

    ; 4.8MHz/8 = 600 kHz
    ; 600 kHz / 256 = 2.3 kHz
    ldi temp, (1<<CS01)
    out TCCR0B, temp

    ; setup PWM
    ldi temp, (1<<COM0A1)|(1<<WGM01)|(1<<WGM00)
    out TCCR0A, temp

    ; brightness=0 for now
    clr temp
    out OCR0A, temp

    ; PB2: off, PB0/OC0A: dim
    ldi temp, (0<<PB2)|(1<<PB0)
    out PORTB, temp
    ldi temp, (1<<PB2)|(1<<PB0)
    out DDRB, temp

    ; enable T0 overflow int
    ldi temp, (1<<TOIE0)
    out TIMSK0, temp

    ; wait for eeprom ready
EEPROM_WAIT:
    sbic EECR, EEPE
    rjmp EEPROM_WAIT

    ; load myaddr. from eeprom
    clr temp
    out EEARL, temp
    sbi EECR, EERE
    in myaddr, EEDR

    ; prepare INT0
    ldi temp, (1<<INT0)
    out GIMSK, temp

    clr ovf
    clr state
    clr flags

    sei
    
    ; enable INT0 & sleep mode
    ldi temp, (1<<SE)|(1<<ISC00)
    out MCUCR, temp
    
    ; TODO: dim up on start
    
LOOP:
    sleep
    rjmp LOOP
