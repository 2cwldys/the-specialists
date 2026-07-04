
reference/client.dll:     file format pei-i386


Disassembly of section .text:

1002fef0 <.text+0x2eef0>:
1002fef0:	56                   	push   esi
1002fef1:	8b 74 24 0c          	mov    esi,DWORD PTR [esp+0xc]
1002fef5:	d9 46 20             	fld    DWORD PTR [esi+0x20]
1002fef8:	d9 e0                	fchs
1002fefa:	d9 05 58 9e 07 10    	fld    DWORD PTR ds:0x10079e58
1002ff00:	d8 d9                	fcomp  st(1)
1002ff02:	df e0                	fnstsw ax
1002ff04:	f6 c4 01             	test   ah,0x1
1002ff07:	75 1b                	jne    0x1002ff24
1002ff09:	d9 05 d8 d4 07 10    	fld    DWORD PTR ds:0x1007d4d8
1002ff0f:	d8 d9                	fcomp  st(1)
1002ff11:	df e0                	fnstsw ax
1002ff13:	f6 c4 41             	test   ah,0x41
1002ff16:	75 0c                	jne    0x1002ff24
1002ff18:	dd d8                	fstp   st(0)
1002ff1a:	c7 44 24 0c 00 00 80 	mov    DWORD PTR [esp+0xc],0xbf800000
1002ff21:	bf 
1002ff22:	eb 1f                	jmp    0x1002ff43
1002ff24:	d9 05 58 9e 07 10    	fld    DWORD PTR ds:0x10079e58
1002ff2a:	d8 d9                	fcomp  st(1)
1002ff2c:	df e0                	fnstsw ax
1002ff2e:	f6 c4 01             	test   ah,0x1
1002ff31:	74 0c                	je     0x1002ff3f
1002ff33:	dd d8                	fstp   st(0)
1002ff35:	c7 44 24 0c 00 00 80 	mov    DWORD PTR [esp+0xc],0x3f800000
1002ff3c:	3f 
1002ff3d:	eb 04                	jmp    0x1002ff43
1002ff3f:	d9 5c 24 0c          	fstp   DWORD PTR [esp+0xc]
1002ff43:	d9 44 24 0c          	fld    DWORD PTR [esp+0xc]
1002ff47:	e8 64 ca 03 00       	call   0x1006c9b0
1002ff4c:	8b 4c 24 08          	mov    ecx,DWORD PTR [esp+0x8]
1002ff50:	d9 c0                	fld    st(0)
1002ff52:	d9 59 04             	fstp   DWORD PTR [ecx+0x4]
1002ff55:	d8 15 f0 99 07 10    	fcom   DWORD PTR ds:0x100799f0
1002ff5b:	d9 c0                	fld    st(0)
1002ff5d:	df e0                	fnstsw ax
1002ff5f:	f6 c4 41             	test   ah,0x41
1002ff62:	74 02                	je     0x1002ff66
1002ff64:	d9 e0                	fchs
1002ff66:	dc 25 e0 d5 07 10    	fsub   QWORD PTR ds:0x1007d5e0
1002ff6c:	dc 1d 78 a8 07 10    	fcomp  QWORD PTR ds:0x1007a878
1002ff72:	df e0                	fnstsw ax
1002ff74:	d8 15 f0 99 07 10    	fcom   DWORD PTR ds:0x100799f0
1002ff7a:	f6 c4 41             	test   ah,0x41
1002ff7d:	df e0                	fnstsw ax
1002ff7f:	75 0f                	jne    0x1002ff90
1002ff81:	f6 c4 41             	test   ah,0x41
1002ff84:	74 02                	je     0x1002ff88
1002ff86:	d9 e0                	fchs
1002ff88:	dc 25 e0 d5 07 10    	fsub   QWORD PTR ds:0x1007d5e0
1002ff8e:	eb 0f                	jmp    0x1002ff9f
1002ff90:	f6 c4 41             	test   ah,0x41
1002ff93:	74 02                	je     0x1002ff97
1002ff95:	d9 e0                	fchs
1002ff97:	dc 25 e0 d5 07 10    	fsub   QWORD PTR ds:0x1007d5e0
1002ff9d:	d9 e0                	fchs
1002ff9f:	dc 1d f0 d5 07 10    	fcomp  QWORD PTR ds:0x1007d5f0
1002ffa5:	df e0                	fnstsw ax
1002ffa7:	f6 c4 01             	test   ah,0x1
1002ffaa:	74 15                	je     0x1002ffc1
1002ffac:	c7 01 00 00 00 00    	mov    DWORD PTR [ecx],0x0
1002ffb2:	d9 46 04             	fld    DWORD PTR [esi+0x4]
1002ffb5:	d9 e0                	fchs
1002ffb7:	d9 46 14             	fld    DWORD PTR [esi+0x14]
1002ffba:	d9 f3                	fpatan
1002ffbc:	5e                   	pop    esi
1002ffbd:	d9 59 08             	fstp   DWORD PTR [ecx+0x8]
1002ffc0:	c3                   	ret
1002ffc1:	d9 46 24             	fld    DWORD PTR [esi+0x24]
1002ffc4:	d9 46 28             	fld    DWORD PTR [esi+0x28]
1002ffc7:	d9 f3                	fpatan
1002ffc9:	d9 19                	fstp   DWORD PTR [ecx]
1002ffcb:	d9 46 10             	fld    DWORD PTR [esi+0x10]
1002ffce:	d9 06                	fld    DWORD PTR [esi]
1002ffd0:	d9 f3                	fpatan
1002ffd2:	5e                   	pop    esi
1002ffd3:	d9 59 08             	fstp   DWORD PTR [ecx+0x8]
1002ffd6:	c3                   	ret
