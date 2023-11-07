EMU_IO_PRINT = $3ff0
EMU_IO_RESET = $3fff

* = $8000
start:
  sei
  cld
  ldx #$ff
  txs

  lda EMU_IO_PRINT              ; read ascii number
  cmp #$30                      ; n < $30
  bmi error                     ; yes? error
  cmp #$39                      ; n > $39
  beq num_cont                  ; n == $39? continue
  bcs error                     ; yes? error

num_cont:
  sec                           ; ascii
  sbc #$30                      ;  to number
  jsr fib                       ; fib(n)
  jsr print_hex                 ; print fib(n)

halt:
  lda #$01
  sta EMU_IO_RESET

error:
  ldx #0
error_loop:
  lda err,x
  beq halt
  sta EMU_IO_PRINT
  inx
  bne error_loop

  jmp halt

  ;; calculate fib(a)
fib:
  cmp #2                        ; n < 2
  bpl fib_cont                  ; no? continue
  beq fib_cont                  ; n == 2? continue
  rts                           ; yes? return n
fib_cont:

  tax                           ; x = n
  dex                           ; x = n - 1
  txa                           ; a = n - 1
  pha                           ; save n - 1
  jsr fib                       ; a = fib(n - 1)
  tay                           ; y = fib(n - 1)
  pla                           ; a = n - 1
  tax                           ; x = n - 1
  dex                           ; x = n - 2
  tya                           ; a = fib(n - 1)
  pha                           ; save fib(n - 1)
  txa                           ; x = n - 2
  jsr fib                       ; a = fib(n - 2)
  clc                           ; clear carry
  sta tmp                       ; save fib(n - 2)
  pla                           ; a = fib(n - 1)
  adc tmp                       ;  + fib(n - 2)

  rts

  ;; print a as hex
print_hex:
  pha
  and #$f0
  lsr
  lsr
  lsr
  lsr
  tax
  lda hex,x
  sta EMU_IO_PRINT

  pla
  and #$0f
  tax
  lda hex,x
  sta EMU_IO_PRINT

  lda #$0a
  sta EMU_IO_PRINT

  rts

hex .byt "0123456789abcdef"
tmp .byt 0
err .byt "Input must be a single digit!",$a,0

.dsb $fffa-*, $00
.word $0000, start, $0000
