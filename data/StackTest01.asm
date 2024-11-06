bits 16

mov ax, 0xfff0
mov ss, ax
mov sp, 0xff

mov bp, 5
mov ax, 10
push ax
push bp
mov bp, sp
mov word bx, [bp + 2] ; Not pushing bx yet. 