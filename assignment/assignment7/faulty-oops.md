# Run echo “hello_world” > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000<br />
Mem abort info:<br />
  ESR = 0x96000045<br />
  EC = 0x25: DABT (current EL), IL = 32 bits<br />
  SET = 0, FnV = 0<br />
  EA = 0, S1PTW = 0<br />
  FSC = 0x05: level 1 translation fault<br />
Data abort info:<br />
  ISV = 0, ISS = 0x00000045<br />
  CM = 0, WnR = 1<br />
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000420e2000<br />
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000<br />
Internal error: Oops: 96000045 [#1] SMP<br />
Modules linked in: hello(O) scull(O) faulty(O)<br />
CPU: 0 PID: 158 Comm: sh Tainted: G           O      5.15.18 #1<br />
Hardware name: linux,dummy-virt (DT)<br />
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)<br />
pc : faulty_write+0x14/0x20 [faulty]<br />
lr : vfs_write+0xa8/0x2b0<br />
sp : ffffffc008d23d80<br />
x29: ffffffc008d23d80 x28: ffffff80020c8000 x27: 0000000000000000<br />
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000<br />
x23: 0000000040001000 x22: 0000000000000012 x21: 0000005571872a70<br />
x20: 0000005571872a70 x19: ffffff80020a1b00 x18: 0000000000000000<br />
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000<br />
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000<br />
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000<br />
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000<br />
x5 : 0000000000000001 x4 : ffffffc0006f0000 x3 : ffffffc008d23df0<br />
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000<br />
Call trace:<br />
 faulty_write+0x14/0x20 [faulty]<br />
 ksys_write+0x68/0x100<br />
 __arm64_sys_write+0x20/0x30<br />
 invoke_syscall+0x54/0x130<br />
 el0_svc_common.constprop.0+0x44/0xf0<br />
 do_el0_svc+0x40/0xa0<br />
 el0_svc+0x20/0x60<br />
 el0t_64_sync_handler+0xe8/0xf0<br />
 el0t_64_sync+0x1a0/0x1a4<br />
Code: d2800001 d2800000 d503233f d50323bf (b900003f) <br />
---[ end trace c684b313d8573ef2 ]---<br />

## Analysis
- It is an oops message genereated by null pointer dereference or by use of incorrect pointer value.Also
 known as page faults.
- Shown by the message "Unable to handle kernel NULL pointer dereference".
- Various register value associated with the fault is shown which is helpful to debug the fault location.
- This message  "faulty_write+0x14/0x20 [faulty]" shows us the fault location, it occured at faulty_write function at (0x10) bytes inside it when the 'faulty' module was loaded.
- There was an attempt to deference a null pointer here.Thus the kernal gave the faulty oops message. The outcome of null pointer derefrencing can be seen in objdump. 