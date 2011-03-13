.include "m8def.inc"

; code needs 16MHz for PWM
; use 14.7456MHz if you need the UART
.equ	F_CPU		= 16000000
;.equ	F_CPU		= 14745600


.equ	BAUD		= 115200

.equ UBRR_VAL   = ((F_CPU+BAUD*8)/(BAUD*16)-1)  ; clever runden
.equ BAUD_REAL  = (F_CPU/(16*(UBRR_VAL+1)))     ; Reale Baudrate
.equ BAUD_ERROR = ((BAUD_REAL*1000)/BAUD-1000)  ; Fehler in Promille

.if ((BAUD_ERROR>10) || (BAUD_ERROR<-10))       ; max. +/-10 Promille Fehler
.warning "baudrate error > 1%, UART disabled"
.endif
; source: http://www.mikrocontroller.net/articles/AVR-Tutorial:_UART

.if (F_CPU != 16000000)
.warning "PWM output may be poor with F_CPU != 16MHz"
.endif

.equ PWM_MAXVAL	= (F_CPU/(4*8000))	; PCM is 8kHz

; references:
;
; http://wwwlehre.dhbw-stuttgart.de/~schulte/digiisdn.htm
; http://www.virtualuniversity.ch/telekom/telefon/7.html
; http://www.rhyshaden.com/isdn.htm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; r0/1: SPM/LPM

.def	t0val		= r2
.def	lastpol		= r3

; immediates
.def	temp		= r16
.def	temp2		= r17

.def	state		= r18
.def	bitcnt		= r19
.def	rxbyte		= r20

; X: 26/27
; Y: 28/29
; Z: 30/31

.equ	ST_NOSYNC	= 0
.equ	ST_F		= 1
.equ	ST_L		= 2
.equ	ST_DATA		= 3

.equ	T0_SVAL		= (F_CPU/192000)

    ; RESET
    rjmp RESET
    ; INT0
    rjmp INT0_ISR
    ; INT1
    rjmp INT1_ISR
    ; OC2
    reti
    ; OVF2
    reti
    ; ICP1
    reti
    ; OC1A
    reti
    ; OC1B
    reti
    ; OVF1
    reti
    ; OVF0
    rjmp OVF0_ISR
    ; SPI
    reti
    ; URXC
    reti
    ; UDRE
    reti
    ; UTXC
    reti
    ; ADCC
    reti
    ; ERDY
    reti
    ; ACI
    reti
    ; TWI
    reti
    ; SPM
    reti
    ; SPMR
    reti


OVF0_ISR:
    ; prepare T0 for next sample
    ; adjust t0val to compensate for
    ; the IRQ->ISR delay (current val of T0CNT)
    ; just add delay to t0val
    in temp2, TCNT0
    add temp2, t0val
    out TCNT0, temp2
    
    rjmp SAMPLE_S0


INT0_ISR:
INT1_ISR:
    ; prepare T0 for next sample
    ; adjust t0val to compensate for
    ; the IRQ->ISR delay (current val of T0CNT)
    ; just add delay to t0val
    out TCNT0, t0val
    
    ; reset T0OVF flag
    nop
    nop
    nop
    ldi temp, (1<<TOV0)
    out TIFR, temp
    
    ; enable T0 int
    ldi temp, (1<<TOIE0)
    out TIMSK, temp
    
SAMPLE_S0:
    in temp, PIND
    andi temp, $0C
    
    ; clear interrupt flags
    ldi temp2, (1<<INTF1)|(1<<INTF0)
    out GIFR, temp2
    
    ; "11" -> '1', else '0'
    cpi temp, $0C
    brne ZERO_BIT
    
    ; '1' databit
    ldi temp, 1
    
    cpi state, ST_L
    brsh RCV_DATABIT

SYNC_LOST:
    clr temp
    out TIMSK, temp
    
SYNC_LOST2:
    clr state
    
    reti
    
ZERO_BIT:
    ; same polarity as last sample?
    ; (code violation - sync candidate)
    cp temp, lastpol
    breq CODE_VIOLATION
    
    ; no code violation
    
    ; save polarity for next sample
    mov lastpol, temp
    
    ; inverse AMI: +-diff is '0' databit
    clr temp
    
    ; normal databit or
    ; F -> L state transition?
    cpi state, ST_L
    brsh RCV_DATABIT
    
    cpi state, ST_F
    brne SYNC_LOST2
    
    ; F -> L state transition

    ; L bit received
    ; -> start data reception
    ldi state, ST_L
    
    ; reset bitcnt (L-bit is #2)
    ldi bitcnt, 2
    
    reti
    
    
CODE_VIOLATION:

    ; same polarity as last sample
    
    ; there's a 2nd code violation for
    ; 1st zero databit after L bit
    ; don't resync in this case
    cpi state, ST_L
    breq SYNC_IGNORE
    
    ; 1st code violation -> resync
    ldi state, ST_F
    
    reti
    
    
SYNC_IGNORE:
    ; 2nd code violation
    ; treat further code violations as sync
    ldi state, ST_DATA
    
    ; inverse AMI: +-diff is '0' databit
    clr temp
    
    
RCV_DATABIT:

    ; check for end of frame
    cpi bitcnt, 48
    brsh RCV_RESET
    
    ; shift in databit
    lsl rxbyte
    add rxbyte, temp
    
    inc bitcnt
    
    ; check if PCM byte
    ; (B1 channel only)
    cpi bitcnt, 10
    breq HANDLE_PCM
    
    ; 2nd B1 byte in frame
    cpi bitcnt, 34
    breq HANDLE_PCM
    
    reti
    
RCV_RESET:
    ldi state, ST_NOSYNC
    reti
    
HANDLE_PCM:

.if ((BAUD_ERROR<=10) && (BAUD_ERROR>=-10))
    ; send raw databyte via UART
    out UDR, rxbyte
.endif
    
    ; rxbyte * 2 for a-law lut
    ; (16 bits per value)
    clr temp
    lsl rxbyte
    rol temp
    
    ; calc a-law lut addr
    ldi ZL, low(ALAW_LUT*2)
    ldi ZH, high(ALAW_LUT*2)
    add ZL, rxbyte
    adc ZH, temp
    
    ; load lut value
    lpm r1, Z+
    lpm r0, Z
    
    ; feed to PWM
    out OCR1AH, r1
    out OCR1AL, r0

    ; LEDs blinky!
    out PORTC, r0

    reti
    
    
RESET:
    ; setup stack 1st
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp
    
    ; enable T0 with F_CPU
    ; for sampling the S0 bus
    ldi temp, (1<<CS00)
    out TCCR0, temp
    
    ; enable T1 with F_CPU
    ; phase+frequency correct PWM
    ; for PCM output
    ldi temp, (1<<COM1A1)
    out TCCR1A, temp
    ldi temp, (1<<WGM13)|(1<<CS10)
    out TCCR1B, temp
    
    ; 16 kHz at F_CPU=16MHz
    ldi temp, high(PWM_MAXVAL)
    ldi temp2, low(PWM_MAXVAL)
    out ICR1H, temp
    out ICR1L, temp2
    
    ; "zero" value
    ldi temp, high(PWM_MAXVAL/2)
    ldi temp2, low(PWM_MAXVAL/2)
    out OCR1AH, temp
    out OCR1AL, temp2
    
    ; enable output buffer for PWM
    sbi DDRB, PB1
    
    ; enable output buffers for LEDs
    ser temp
    out DDRC, temp
    
    ; INT0/1 falling edges
    ldi temp, (1<<ISC01)|(1<<ISC11)
    out MCUCR, temp
    ldi temp, (1<<INT0)|(1<<INT1)
    out GICR, temp
    
    clr state
    clr lastpol
    
    ; prepare t0vals for S0 sampling
    ; single databit distance
    ldi temp, low(256 - T0_SVAL)
    mov t0val, temp
    
.if ((BAUD_ERROR<=10) && (BAUD_ERROR>=-10))
    ; UART init
    ldi temp2, high(UBRR_VAL)
    ldi temp, low(UBRR_VAL)
    out UBRRH, temp2
    out UBRRL, temp
    ldi temp, (1<<TXEN)
    out UCSRB, temp
    ldi temp, (1<<UCSZ1)|(1<<UCSZ0)|(1<<URSEL)
    out UCSRC, temp
.endif

    sei
    
MAIN:
    rjmp MAIN

ALAW_LUT:
.include "alaw.inc"

.if (PWM_MAXVAL != LUT_MAXVAL)
.warning "a-law LUT doesn't match F_CPU"
.warning "PWM output will be broken"
.warning "regenerate LUT with ./alaw_gentab <F_CPU> > alaw.inc"
.endif
