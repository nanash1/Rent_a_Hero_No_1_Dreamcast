					.cpu		SH4
					.endian		little
;					.output		dbg
;
; Modified drawing function to call f_write_char_hw
;
					.section main,code,locate=h'8c046c9c;
					mov.l   r14, @-r15
					mov.l   r12, @-r15
					mov.l   r11, @-r15
					mov.l   r10, @-r15
					mov.l   r9, @-r15
					sts.l   pr, @-r15
					mov.l   r4, @-r15
					mov		#0, r10
					mov.l   r10, @-r15
					mov.l   literal+4, r10
					mov.l	literal+24,r11
					mov     r7, r12
					mov     r5, r14
					bra     label2
				 	mov     r6, r9
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
					mov     r14, r6
					mov.w   @r12+, r5
					mov     r9, r7
					jsr		@r11
					mov.l   @(4,r15), r4
					add     #3, r14
					mov     #h'1E, r2
					cmp/ge  r2, r14
					bf    	label2
					add     #3, r9
					mov     #0, r14
label2:
					mov.w   @r12, r3
					extu.w  r3, r3
					cmp/eq  r10, r3
					bf      loop
					mov     #0, r0
					add		#8, r15
					lds.l   @r15+, pr
					mov.l   @r15+, r9
					mov.l   @r15+, r10
					mov.l   @r15+, r11
					mov.l   @r15+, r12
					rts
					mov.l   @r15+, r14
					.end	