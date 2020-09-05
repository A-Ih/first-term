                section         .text

                global          _start
_start:

                lea             rsp, [rsp - 3 * long_size * 8]  ; three chuncks with 128 portions of 8 bytes
                mov             rcx, long_size
                lea             rdi, [rsp + rcx * 8]  ; two chuncks for hi- and lo- parts
                call            read_long  ; lo- part is read
                mov             rdi, rsp
                call            read_long
                mov             rsi, rdi
                lea             rdi, [rsp + rcx * 8]  ; pointing to big chunk again
                call            mul_long_long
                add             rcx, rcx  ; write both parts
                call            write_long

                mov             al, 0x0a
                call            write_char

                jmp             exit

; multiplies two long numbers
;    rdi -- address of factor #1 (points to lo- part followed by hi-)
;    rsi -- address of factor #2 (long number)
;    rcx -- length of long numbers in qwords
; result:
;    product (lo- and hi- parts) is written to rdi
mul_long_long:
    push            rdi
    push            rsi
    push            rcx
 
        lea             rdi, [rdi + rcx * 8]
        call            set_zero  ; set hi- part to zero
        mov             r8, rcx  ; length of the number
        lea             rcx, [rcx * 8]
        sub             rdi, rcx

;  length   -- r8
;  factor1  -- r9
;  factor2  -- r10
;  buffer1  -- r11
;  buffer2  -- r12

        mov             r9, rdi  ; first factor (2 * length * 8)
        mov             r10, rsi  ; second factor (length * 8)

        add             rcx, rcx
        sub             rsp, rcx
        mov             r11, rsp  ; first buffer (holds initial value of first factor)
        lea             r12, [rsp + r8 * 8]  ; second buffer (is being multiplied)

        mov             rsi, r9
        mov             rdi, r11  ; rdi points to the beginning of buffer1
        mov             rcx, r8
        call            long_copy  ; copied factor1 to buffer1

        mov             rdi, r9
        lea             rcx, [2 * r8]
        call            set_zero  ; set destination to zero

;  offset   -- r13

        xor             r13, r13

        .loop:
            ;  copy(from : buf1, to : buf2, length)
            mov             rsi, r11
            mov             rdi, r12
            mov             rcx, r8
            call            long_copy

            ;  mul_long_short(buf2, factor2[offset], length)
            mov             rdi, r12
            mov             rbx, [r10 + r13 * 8]
            mov             rcx, r8
            call            mul_long_short

;  carry  -- r14

            mov             r14, rsi

            ;  add_long_long([factor1 + offset * 8], buf2, length)
            lea             rdi, [r9 + r13 * 8]
            mov             rsi, r12
            mov             rcx, r8
            call            add_long_long
            ;  save the carry bit from addition
            adc             r14, 0

            ;  if (carry != 0) add_long_short([factor1 + 8 * (offset + length)], carry, length - offset)
            test            r14, r14
            jz              .no_carry
                lea             rdi, [r9 + 8 * r13]
                lea             rdi, [rdi + 8 * r8]
                mov             rax, r14
                mov             rcx, r8
                sub             rcx, r13
                call            add_long_short
            .no_carry:

            inc             r13
            cmp             r13, r8
        jne             .loop

        mov             rcx, r8
        add             rcx, rcx
        lea             rsp, [rsp + rcx * 8]

    pop             rcx
    pop             rsi
    pop             rdi
    ret

; transfers long number from rsi to rdi
;    rsi -- source (long number)
;    rdi -- destination (long number)
;    rcx -- length of long number in qwords
long_copy:
                push            rcx
                rep movsq
                pop             rcx
                ret


; adds two long number
;    rdi -- address of summand #1 (long number)
;    rsi -- address of summand #2 (long number)
;    rcx -- length of long numbers in qwords
; result:
;    sum is written to rdi
add_long_long:
                push            rdi
                push            rsi
                push            rcx

                clc
.loop:
                mov             rax, [rsi]
                lea             rsi, [rsi + 8]
                adc             [rdi], rax
                lea             rdi, [rdi + 8]
                dec             rcx  ; dec doesn't disturb CF
                jnz             .loop

                pop             rcx
                pop             rsi
                pop             rdi
                ret

; adds 64-bit number to long number
;    rdi -- address of summand #1 (long number)
;    rax -- summand #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    sum is written to rdi
add_long_short:
                push            rdi
                push            rcx
                push            rdx

                xor             rdx,rdx
.loop:
                add             [rdi], rax  ; rax - carry from previous summation
                adc             rdx, 0
                mov             rax, rdx
                xor             rdx, rdx
                add             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rdx
                pop             rcx
                pop             rdi
                ret

; multiplies long number by a short
;    rdi -- address of multiplier #1 (long number)
;    rbx -- multiplier #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    product is written to rdi
;    last carry is written to rsi
mul_long_short:
                push            rax
                push            rdi
                push            rcx

                xor             rsi, rsi
.loop:
                mov             rax, [rdi]
                mul             rbx  ; rax * rbx -> rdx:rax
                add             rax, rsi
                adc             rdx, 0  ; add carry to the 'hi' part
                mov             [rdi], rax
                add             rdi, 8
                mov             rsi, rdx
                dec             rcx
                jnz             .loop

; rsi now has the last carry, we can use it

                pop             rcx
                pop             rdi
                pop             rax
                ret

; divides long number by a short
;    rdi -- address of dividend (long number)
;    rbx -- divisor (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    quotient is written to rdi
;    rdx -- remainder
div_long_short:
                push            rdi
                push            rax
                push            rcx

                lea             rdi, [rdi + 8 * rcx - 8]  ; address of the last qword
                xor             rdx, rdx

.loop:
                mov             rax, [rdi]
                div             rbx  ; rdx:rax / rbx -> rax - quot, rdx - rem
                mov             [rdi], rax
                sub             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rax
                pop             rdi
                ret

; assigns a zero to long number
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
set_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep stosq

                pop             rcx
                pop             rdi
                pop             rax
                ret

; checks if a long number is a zero
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
; result:
;    ZF=1 if zero
is_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep scasq  ; maybe there should be REPE

                pop             rcx
                pop             rdi
                pop             rax
                ret

; read long number from stdin
;    rdi -- location for output (long number)
;    rcx -- length of long number in qwords
read_long:
                push            rcx
                push            rdi

                call            set_zero
.loop:
                call            read_char
                or              rax, rax  ; -1 if problem occurred
                js              exit
                cmp             rax, 0x0a  ; line feed
                je              .done
                cmp             rax, '0'
                jb              .invalid_char
                cmp             rax, '9'
                ja              .invalid_char

                sub             rax, '0'
                mov             rbx, 10
                call            mul_long_short
                call            add_long_short
                jmp             .loop

.done:
                pop             rdi
                pop             rcx
                ret

.invalid_char:
                mov             rsi, invalid_char_msg
                mov             rdx, invalid_char_msg_size
                call            print_string
                call            write_char
                mov             al, 0x0a
                call            write_char

.skip_loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              exit
                jmp             .skip_loop

; write long number to stdout
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
write_long:
                push            rax
                push            rcx

                mov             rax, 20
                mul             rcx
                mov             rbp, rsp
                sub             rsp, rax

                mov             rsi, rbp

.loop:
                mov             rbx, 10
                call            div_long_short
                add             rdx, '0'
                dec             rsi
                mov             [rsi], dl
                call            is_zero
                jnz             .loop

                mov             rdx, rbp
                sub             rdx, rsi
                call            print_string

                mov             rsp, rbp
                pop             rcx
                pop             rax
                ret

; read one char from stdin
; result:
;    rax == -1 if error occurs
;    rax \in [0; 255] if OK
read_char:
                push            rcx
                push            rdi

                sub             rsp, 1  ; one byte for a char
                xor             rax, rax  ; 0 - sys_read
                xor             rdi, rdi  ; 0 - stdin
                mov             rsi, rsp  ; char *buf = rsp
                mov             rdx, 1  ; size of buffer = 1
                syscall

                cmp             rax, 1  ; number of bytes read
                jne             .error
                xor             rax, rax
                mov             al, [rsp]  ; result in al
                add             rsp, 1

                pop             rdi
                pop             rcx
                ret
.error:
                mov             rax, -1
                add             rsp, 1
                pop             rdi
                pop             rcx
                ret

; write one char to stdout, errors are ignored
;    al -- char
write_char:
                sub             rsp, 1
                mov             [rsp], al

                mov             rax, 1
                mov             rdi, 1
                mov             rsi, rsp
                mov             rdx, 1
                syscall
                add             rsp, 1
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

; print string to stdout
;    rsi -- string
;    rdx -- size
print_string:
                push            rax

                mov             rax, 1
                mov             rdi, 1
                syscall

                pop             rax
                ret


                section         .rodata

long_size:      equ             128
invalid_char_msg:
                db              "Invalid character: "
invalid_char_msg_size: equ             $ - invalid_char_msg
