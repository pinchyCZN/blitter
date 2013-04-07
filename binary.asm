;nasmw -t -f  win32 -o$(IntDir)\$(InputName).obj -Xvc $(InputName).asm

SECTION .data


global _bitmapfonts

_bitmapfonts:
; incbin "charset.bin"
 incbin "vga737.bin"
_bitmapfontsend:


global _bitmapfile

_bitmapfile:
; incbin "untitled.bmp"