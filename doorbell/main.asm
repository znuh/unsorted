.include "tn13def.inc"

; fuses: lfuse:0x67 hfuse:0xfb
; avrdude -cstk500v2 -P /dev/ttyUSB0 -p t13 -U flash:w:main.hex -U lfuse:w:0x67:m -U hfuse:w:0xfb:m

; clock: 128kHz / 8 = 16kHz

; current during sleep is ~30uA
; 2200uF @4.7V is good for 2-3 minutes

.def	temp	= r16
.def	temp2	= r17

.def	ringcnt	= r18
.def	ovfcnt	= r19

; X: 26/27
; Y: 28/29
; Z: 30/31

	; RESET
	rjmp RESET
	; INT0
	reti
	; PCINT0
	rjmp PCINT
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
	; >= 1 minute since last ring?
	cpi ovfcnt, 3
	brsh TIMEOUT

	inc ovfcnt
	reti

TIMEOUT:
	clr ringcnt
	reti

PCINT:
	; ringing signal present?
	sbis PINB, PB1
	rjmp RING_END

	; ringing starts

	; reset timeout
	clr ovfcnt

	; ring after 2nd time
	cpi ringcnt, 2
	brsh RING

	inc ringcnt
	reti

RING:
	sbi PORTB, PB0
	reti

RING_END:
	; disable NMOS/relais
	; saves some power (100k pulldown on NMOS gate)
	cbi PORTB, PB0
	reti

; PB1: power detect
; PB0: NMOS/relais enable

RESET:
	; enable output (low) for PB0 - disable relais
	sbi DDRB, PB0

	; heavy power-saving ahead

	; enable pullups for all IOs except PB1/PB0
	ldi temp, $3C
	out PORTB, temp

	; input disable for all IOs except for PB1 (power detect)
	ldi temp, $3D
	out DIDR0, temp

	; disable analog comparator
	sbi ACSR, ACD

	; setup stack pointer
	ldi temp, low(RAMEND)
	out SPL, temp

	; configure timer0
	; 16kHz/1024 = 15.6Hz
	; -> overflow after ~16.4secs
	ldi temp, (1<<CS02)|(1<<CS00)
	out TCCR0B, temp

	; prepare for idle mode
	ldi temp, (1<<SE)
	out MCUCR, temp

	; enable T0 OVF int
	ldi temp, (1<<TOIE0)
	out TIMSK0, temp

	; enable PCINT1
	sbi PCMSK, PCINT1

	; enable pin change int
	ldi temp, (1<<PCIE)
	out GIMSK, temp

	ldi ringcnt, 1
	clr ovfcnt

	sei

LOOP:
	sleep
	rjmp LOOP
