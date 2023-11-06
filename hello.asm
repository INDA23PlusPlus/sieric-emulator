;; can be assembled using xa (https://www.floodgap.com/retrotech/xa/)

* = $8000
start:
  ldx #0
print:
  lda str,x
  beq halt
  sta $3ff0
  inx
  bne print

halt:
  lda #$01
  sta $3fff

str .byt "Hello, world!",$a,0

;; fill with zeroes until interrupt vectors
.dsb $fffa-*, $00
.word $0000      ; NMI vector
.word start       ; reset vector
.word $0000      ; IRQ/BRK vector
