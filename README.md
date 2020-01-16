# max

The code generator is limited currently in functionality.
Function ac_sosDIGIInitDriver in wrappers.S needs to be replaced with the below code:

```Assembly
ac_sosDIGIInitDriver: /* w xipipipi */
		push	%ebp
		mov	%esp,%ebp

		push	%ecx
		push	%edx

		push	0x14(%ebp)
		push	0x10(%ebp)
		push	0xC(%ebp)
		push	0x8(%ebp)

		push	%ecx
		push	%ebx
		push	%edx
		push	%eax

		call	sosDIGIInitDriver
		add	$0x20,%esp

		pop	%edx
		pop	%ecx

		leave
		ret
```
