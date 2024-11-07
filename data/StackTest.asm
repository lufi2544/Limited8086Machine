bits 16

mov ax, 0xfff0
mov ss, ax
mov sp, 0xff


mov bx, 100
mov cx, 200
mov bp, 300
mov ax, 1000
push ax

mov ax, 10
push ax
mov ax, 20
push ax
call FUNCTION1 

pop ax
mov cx, bx
call END


FUNCTION1:					; FUNCTION WITH 2 PARAMETERS
push bp					   ; EPILOG
mov bp, sp
push cx
push bx
mov cx, [bp + 4] ; param1 
mov bx, [bp + 6] ; param2 
				 ; here we would have the Body, doing stuff with local variables
sub sp, 4
mov word [bp - 6], 10
mov ax, [bp - 6]
mov word [bp - 8], 20
mov ax, [bp - 8]

add sp, 4
pop bx
pop cx
pop bp
ret 4

END:
mov ax,0
