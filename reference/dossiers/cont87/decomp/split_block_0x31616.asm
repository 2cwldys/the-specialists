
reference/client.dll:     file format pei-i386


Disassembly of section .text:

10031500 <.text+0x30500>:
10031500:	57                   	push   edi
10031501:	50                   	push   eax
10031502:	8b ce                	mov    ecx,esi
10031504:	ff 52 18             	call   DWORD PTR [edx+0x18]
10031507:	8b 4e 38             	mov    ecx,DWORD PTR [esi+0x38]
1003150a:	8b 16                	mov    edx,DWORD PTR [esi]
1003150c:	8b 89 80 01 00 00    	mov    ecx,DWORD PTR [ecx+0x180]
10031512:	51                   	push   ecx
10031513:	50                   	push   eax
10031514:	57                   	push   edi
10031515:	68 c0 91 0e 10       	push   0x100e91c0
1003151a:	68 6c a8 0e 10       	push   0x100ea86c
1003151f:	8b ce                	mov    ecx,esi
10031521:	ff 52 4c             	call   DWORD PTR [edx+0x4c]
10031524:	8b 56 44             	mov    edx,DWORD PTR [esi+0x44]
10031527:	89 6c 24 10          	mov    DWORD PTR [esp+0x10],ebp
1003152b:	39 aa 8c 00 00 00    	cmp    DWORD PTR [edx+0x8c],ebp
10031531:	0f 8e 3f 01 00 00    	jle    0x10031676
10031537:	89 5c 24 14          	mov    DWORD PTR [esp+0x14],ebx
1003153b:	33 d2                	xor    edx,edx
1003153d:	bf 34 4d 09 10       	mov    edi,0x10094d34
10031542:	8b c3                	mov    eax,ebx
10031544:	8a 18                	mov    bl,BYTE PTR [eax]
10031546:	8a cb                	mov    cl,bl
10031548:	3a 1f                	cmp    bl,BYTE PTR [edi]
1003154a:	75 1c                	jne    0x10031568
1003154c:	84 c9                	test   cl,cl
1003154e:	74 14                	je     0x10031564
10031550:	8a 58 01             	mov    bl,BYTE PTR [eax+0x1]
10031553:	8a cb                	mov    cl,bl
10031555:	3a 5f 01             	cmp    bl,BYTE PTR [edi+0x1]
10031558:	75 0e                	jne    0x10031568
1003155a:	83 c0 02             	add    eax,0x2
1003155d:	83 c7 02             	add    edi,0x2
10031560:	84 c9                	test   cl,cl
10031562:	75 e0                	jne    0x10031544
10031564:	33 c0                	xor    eax,eax
10031566:	eb 05                	jmp    0x1003156d
10031568:	1b c0                	sbb    eax,eax
1003156a:	83 d8 ff             	sbb    eax,0xffffffff
1003156d:	85 c0                	test   eax,eax
1003156f:	0f 84 a1 00 00 00    	je     0x10031616
10031575:	8b 44 24 14          	mov    eax,DWORD PTR [esp+0x14]
10031579:	bf 28 4d 09 10       	mov    edi,0x10094d28
1003157e:	8a 18                	mov    bl,BYTE PTR [eax]
10031580:	8a cb                	mov    cl,bl
10031582:	3a 1f                	cmp    bl,BYTE PTR [edi]
10031584:	75 1c                	jne    0x100315a2
10031586:	84 c9                	test   cl,cl
10031588:	74 14                	je     0x1003159e
1003158a:	8a 58 01             	mov    bl,BYTE PTR [eax+0x1]
1003158d:	8a cb                	mov    cl,bl
1003158f:	3a 5f 01             	cmp    bl,BYTE PTR [edi+0x1]
10031592:	75 0e                	jne    0x100315a2
10031594:	83 c0 02             	add    eax,0x2
10031597:	83 c7 02             	add    edi,0x2
1003159a:	84 c9                	test   cl,cl
1003159c:	75 e0                	jne    0x1003157e
1003159e:	33 c0                	xor    eax,eax
100315a0:	eb 05                	jmp    0x100315a7
100315a2:	1b c0                	sbb    eax,eax
100315a4:	83 d8 ff             	sbb    eax,0xffffffff
100315a7:	85 c0                	test   eax,eax
100315a9:	74 6b                	je     0x10031616
100315ab:	8d 85 6c a8 0e 10    	lea    eax,[ebp+0x100ea86c]
100315b1:	8d 8d 90 67 0e 10    	lea    ecx,[ebp+0x100e6790]
100315b7:	8b 5c 24 14          	mov    ebx,DWORD PTR [esp+0x14]
100315bb:	83 c5 0c             	add    ebp,0xc
100315be:	8b 38                	mov    edi,DWORD PTR [eax]
100315c0:	83 c3 70             	add    ebx,0x70
100315c3:	89 39                	mov    DWORD PTR [ecx],edi
100315c5:	89 5c 24 14          	mov    DWORD PTR [esp+0x14],ebx
100315c9:	8b 78 04             	mov    edi,DWORD PTR [eax+0x4]
100315cc:	89 79 04             	mov    DWORD PTR [ecx+0x4],edi
100315cf:	8b 40 08             	mov    eax,DWORD PTR [eax+0x8]
100315d2:	89 41 08             	mov    DWORD PTR [ecx+0x8],eax
100315d5:	8d 8a c0 91 0e 10    	lea    ecx,[edx+0x100e91c0]
100315db:	8d 82 c0 89 0e 10    	lea    eax,[edx+0x100e89c0]
100315e1:	83 c2 10             	add    edx,0x10
100315e4:	8b 39                	mov    edi,DWORD PTR [ecx]
100315e6:	89 38                	mov    DWORD PTR [eax],edi
100315e8:	8b 79 04             	mov    edi,DWORD PTR [ecx+0x4]
100315eb:	89 78 04             	mov    DWORD PTR [eax+0x4],edi
100315ee:	8b 79 08             	mov    edi,DWORD PTR [ecx+0x8]
100315f1:	89 78 08             	mov    DWORD PTR [eax+0x8],edi
100315f4:	8b 49 0c             	mov    ecx,DWORD PTR [ecx+0xc]
100315f7:	89 48 0c             	mov    DWORD PTR [eax+0xc],ecx
100315fa:	8b 4e 44             	mov    ecx,DWORD PTR [esi+0x44]
100315fd:	8b 44 24 10          	mov    eax,DWORD PTR [esp+0x10]
10031601:	8b b9 8c 00 00 00    	mov    edi,DWORD PTR [ecx+0x8c]
10031607:	40                   	inc    eax
10031608:	3b c7                	cmp    eax,edi
1003160a:	89 44 24 10          	mov    DWORD PTR [esp+0x10],eax
1003160e:	0f 8c 29 ff ff ff    	jl     0x1003153d
10031614:	eb 5c                	jmp    0x10031672
10031616:	8d 54 24 28          	lea    edx,[esp+0x28]
1003161a:	8d 44 24 34          	lea    eax,[esp+0x34]
1003161e:	52                   	push   edx
1003161f:	50                   	push   eax
10031620:	e8 bb e9 ff ff       	call   0x1002ffe0
10031625:	8d 4c 24 24          	lea    ecx,[esp+0x24]
10031629:	51                   	push   ecx
1003162a:	68 c0 89 0e 10       	push   0x100e89c0
1003162f:	e8 ac e9 ff ff       	call   0x1002ffe0
10031634:	8b 7c 24 20          	mov    edi,DWORD PTR [esp+0x20]
10031638:	8d 54 24 2c          	lea    edx,[esp+0x2c]
1003163c:	d9 44 24 40          	fld    DWORD PTR [esp+0x40]
10031640:	d8 64 24 34          	fsub   DWORD PTR [esp+0x34]
10031644:	c1 e7 04             	shl    edi,0x4
10031647:	52                   	push   edx
10031648:	8d bf c0 89 0e 10    	lea    edi,[edi+0x100e89c0]
1003164e:	d9 5c 24 28          	fstp   DWORD PTR [esp+0x28]
10031652:	57                   	push   edi
10031653:	e8 88 e9 ff ff       	call   0x1002ffe0
10031658:	d9 44 24 34          	fld    DWORD PTR [esp+0x34]
1003165c:	d8 44 24 2c          	fadd   DWORD PTR [esp+0x2c]
10031660:	8d 44 24 34          	lea    eax,[esp+0x34]
10031664:	57                   	push   edi
10031665:	50                   	push   eax
10031666:	d9 5c 24 3c          	fstp   DWORD PTR [esp+0x3c]
1003166a:	e8 61 e5 ff ff       	call   0x1002fbd0
1003166f:	83 c4 20             	add    esp,0x20
10031672:	8b 5c 24 18          	mov    ebx,DWORD PTR [esp+0x18]
10031676:	8b 4e 44             	mov    ecx,DWORD PTR [esi+0x44]
10031679:	33 ff                	xor    edi,edi
1003167b:	89 7c 24 10          	mov    DWORD PTR [esp+0x10],edi
1003167f:	39 b9 8c 00 00 00    	cmp    DWORD PTR [ecx+0x8c],edi
10031685:	0f 8e 12 01 00 00    	jle    0x1003179d
1003168b:	bd 94 67 0e 10       	mov    ebp,0x100e6794
10031690:	c7 44 24 14 c0 89 0e 	mov    DWORD PTR [esp+0x14],0x100e89c0
10031697:	10 
10031698:	83 c3 20             	add    ebx,0x20
1003169b:	8b 44 24 14          	mov    eax,DWORD PTR [esp+0x14]
1003169f:	8d                   	.byte 0x8d
