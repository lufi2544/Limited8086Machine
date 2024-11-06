bits 16

mov ax, 0xfff0
mov ss, ax
mov sp, 0xff

mov bx, 100
mov cx, 200

mov ax, 10
push ax
mov ax, 20
push ax
call FUNCTION1 

mov cx, bx

FUNCTION1:					; FUNCTION WITH 2 PARAMETERS
push bp					   ; EPILOG
mov bp, sp
push cx
push bx
mov cx, [bp + 4] ; param1 
mov bx, [bp + 6] ; param2 
pop bx
pop cx
ret 4
