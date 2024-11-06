bits 16

mov ax, 0xfff0
mov ss, ax
mov sp, 0xff

mov bx, 100
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
sub sp, 4 ; Local Variables 

mov byte [bp - 6], 4 		; BODY
mov word [bp - 8], 360

add sp, 4 				   ; PROLOG
pop bx
pop cx
ret 4
