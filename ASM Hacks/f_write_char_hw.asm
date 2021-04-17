					.cpu		SH4
					.endian		little
;					.output		dbg
;
; Modified routine to load characters into a line buffer
; 
; Arguments
p_lbuff: 			.reg r11
char_idx:			.reg r12
block_pos_x:		.reg r13
block_pos_y:		.reg r9
; Variables
is_even:			.reg r10
p_char:				.reg r4
p_buff:				.reg r5
ext_bits:			.reg r6
cnt					.reg r8
p_twiddle_table		.reg r14
div_5:				.assign h'55555556				; magic number to divide by 3
twiddle_table		.assign h'8c0dd194
p_font_cg			.assign h'8ca00000
					.section main,code,locate=h'8c0b4000;
f_write_char_hw:
					mov.b	@r15, r2
					mov.l   r14, @-r15
					mov.l   r13, @-r15
					mov.l   r12, @-r15
					mov.l   r11, @-r15
					mov.l   r10, @-r15
					mov.l   r9, @-r15
					mov.l   r8, @-r15
					sts.l   pr, @-r15
					add		#-8, r15
					mov.l	r4, p_lbuff
					mov.l	r5, char_idx
					mov.l	r6, block_pos_x
					; divide block positions by 3
					mov.l 	#div_5, r0
					dmulu.l	r0, block_pos_x
					sts 	mach, block_pos_x
					mov.l	r7, block_pos_y
					; multiply char index by 144
					mov.w 	#h'3fff, r3
					and		r3, char_idx
					mov.l	char_idx, r0
					mov.l	r0, p_char
					shll2	r0
					shll	r0
					add		p_char, r0
					shll2	r0
					shll2	r0
					mov.l	#p_font_cg, p_char
					add		r0, p_char
					mov.l	p_char, @r15
					mov.l	r2, @(4,r15)
					; adjust x position to 1.5 block width
				 	mov.l	block_pos_x, r0
					and		#1, r0								; if block x position is odd/even
					mov		r0, is_even
					mov.l	block_pos_x, r0
					shlr	r0
					add		r0, block_pos_x
					; get base offset from table
					mov.l 	#twiddle_table, p_twiddle_table
					mov.b	#9, cnt								; loop through 9 half blocks
					; get lut for line buffer position calculations
p_xytable:			.reg r12
					tst		is_even, is_even
					mov.l	literal+4, p_xytable				; load p_xytable
					bf		loop_write_hblk
					mov.l	literal, p_xytable
loop_write_hblk:
					; get twiddled x-position
					mov.b	@p_xytable+, r0
					add		block_pos_x, r0
					shll2	r0
					mov.l   @(r0, p_twiddle_table), r1
					shll	r1
					; get twiddled y-position
					mov.b	@p_xytable+, r0
					add		block_pos_y, r0
					shll2	r0
					mov.l	@(r0,p_twiddle_table), r2
					add		r2, r1
					mov.b	@p_xytable+, r0
					; calculate line buffer position
					add		p_lbuff, r1							; offset line buffer by half a character depending on odd or even
					and		#16, r0
					add		r0, r1
					; load arguments for f_writeblock
					mov.l	@r15, r3
					mov.l	@(4,r15), ext_bits
					mov		r3, p_char
					mov.b	@p_xytable+, r0						; advance char position to correct part
					add		r0, r3
					mov.l	r3, @r15
					bsr		f_write_hblock 						; call f_writeblock
					mov.l	r1, p_buff
					dt		cnt
					bf/s	loop_write_hblk
					nop
; Return
					add		#8, r15
					lds.l   @r15+, pr
					mov.l   @r15+, r8
					mov.l   @r15+, r9
					mov.l   @r15+, r10
					mov.l   @r15+, r11
					mov.l   @r15+, r12
					mov.l   @r15+, r13
					rts
					mov.l   @r15+, r14
;
					.align 4
literal:
					.data.l xytable_even
					.data.l xytable_odd
;
					.section data1,data,locate=h'8c0b5000
xytable_even:
					; x-positin, y-position, advance line buffer mask, character offset 
					.data.b	h'00,h'00,h'00,8
					.data.b	h'00,h'00,h'ff,8
					.data.b h'00,h'01,h'00,8
					.data.b h'00,h'01,h'ff,8
					.data.b h'01,h'00,h'00,16
					.data.b h'01,h'01,h'00,16
					.data.b	h'00,h'02,h'00,8
					.data.b	h'00,h'02,h'ff,8
					.data.b	h'01,h'02,h'00,8
					.align 4
xytable_odd:	
					.data.b	h'00,h'00,h'ff,16
					.data.b h'00,h'01,h'ff,-8
					.data.b h'01,h'00,h'00,24
					.data.b h'01,h'00,h'ff,-8
					.data.b h'01,h'01,h'00,24
					.data.b h'01,h'01,h'ff,16
					.data.b	h'00,h'02,h'ff,8
					.data.b	h'01,h'02,h'00,8
					.data.b	h'01,h'02,h'ff,8
					.align 4
;
; Write half a block to the line buffer
;
; Arguments
;p_char: 			.reg r4
p_decoded:			.reg r5
;ext_bits:			.reg r6
					.section main,code;
f_write_hblock:
					mov.l   r14, @-r15
					extu.b  r6, r7
					shll2   r7
					extu.b  r7, r3
					shll2   r3
					shll2   r3
					mov     #8, r14
					mov     #3, r6
					or      r3, r7
f_write_block_loop:
					mov.b   @p_char, r0
					extu.b  r0, r0
					shlr2   r0
					shlr2   r0
					shlr2   r0
					and     #3, r0
					mov.b   r0, @p_decoded
					mov.b   @p_char, r2
					mov.b   @p_decoded, r1
					extu.b  r2, r2
					shlr2   r2
					shlr2   r2
					and     r6, r2
					shll2   r2
					shll2   r2
					or      r2, r1
					mov.b   r1, @p_decoded
					mov.b   @p_decoded, r2
					add     r7, r2
					mov.b   r2, @p_decoded
					add     #1, p_decoded
					mov.b   @p_char, r0
					extu.b  r0, r0
					shlr2   r0
					and     #3, r0
					mov.b   r0, @p_decoded
					mov.b   @p_char+, r3
					mov.b   @p_decoded, r2
					extu.b  r3, r3
					and     r6, r3
					shll2   r3
					shll2   r3
					or      r3, r2
					mov.b   r2, @p_decoded
					add     #-1, r14
					mov.b   @p_decoded, r2
					tst     r14, r14
					add     r7, r2
					mov.b   r2, @p_decoded
					bf/s    f_write_block_loop
					add     #1, p_decoded
					rts
					mov.l   @r15+, r14
;
;	
					.end