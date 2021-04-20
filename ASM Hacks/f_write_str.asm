					.cpu		SH4
					.endian		little
;					.output		dbg
;
; Writes a string to a line buffer. Modified to draw half wide characters by calling f_write_char_hw
;
; Arguments
; p_line_buff
; x_pos
; y_pos
; p_str
;
; Variables
p_str:					.reg	r12
x_pos:					.reg	r14
y_pos:					.reg	r9
p_line_buff:				.reg	r4
f_write_char_hw:			.reg	r11
					.section main,code,locate=h'8c046c9c;
f_write_str:
					mov.l   r14, @-r15
					mov.l   r12, @-r15
					mov.l   r11, @-r15
					mov.l   r10, @-r15
					mov.l   r9, @-r15
					sts.l   pr, @-r15
					mov.l   r4, @-r15
					mov	#0, r10
					mov.l   r10, @-r15
					mov.l   literal+4, r10
					mov.l	literal+24,f_write_char_hw
					mov     r7, p_str
					mov     r5, x_pos
					bra     label2
				 	mov     r6, y_pos
					nop
literal:
					.data.w	h'4000
					.data.w	h'1ff
					.data.l	h'8000
					.data.l	h'8c1befe0
					.data.l	h'fe00
					.data.l	h'9000
					.data.l	h'8c1be634
					.data.l	h'8c0b4000
loop:
					mov     x_pos, r6
					mov.w   @p_str+, r5
					mov     y_pos, r7
					jsr	@f_write_char_hw
					mov.l   @(4,r15), r4
					add     #3, x_pos
					mov     #h'1E, r2
					cmp/ge  r2, x_pos
					bf    	label2
					add     #3, y_pos
					mov     #0, x_pos
label2:
					mov.w   @p_str, r3
					extu.w  r3, r3
					cmp/eq  r10, r3
					bf      loop
					mov     #0, r0
					add	#8, r15
					lds.l   @r15+, pr
					mov.l   @r15+, r9
					mov.l   @r15+, r10
					mov.l   @r15+, r11
					mov.l   @r15+, r12
					rts
					mov.l   @r15+, r14
					.end	
