.section .text
.global switch_task
switch_task:
	pushal
    pushfl
    movl %cr3, %eax #Push CR3
    pushl %eax
    movl 44(%esp), %eax #The first argument, where to save
    movl %ebx, 4(%eax)
    movl %ecx, 8(%eax)
    movl %edx, 12(%eax)
    movl %esi, 16(%eax)
    movl %edi, 20(%eax)
    movl 36(%esp), %ebx #EAX
    movl 40(%esp), %ecx #IP
    movl 20(%esp), %edx #ESP
    addl $4, %edx #Remove the return value 
    movl 16(%esp), %esi #EBP
    movl 4(%esp), %edi #EFLAGS
    movl %ebx, (%eax)
    movl %edx, 24(%eax)
    movl %esi, 28(%eax)
    movl %ecx, 32(%eax)
    movl %edi, 36(%eax)
    popl %ebx #CR3
    movl %ebx, 40(%eax)
    pushl %ebx #Goodbye again 
    movl 48(%esp), %eax #Now it is the new object
    movl 4(%eax), %ebx #EBX
    movl 8(%eax), %ecx #ECX
    movl 12(%eax), %edx #EDX
    movl 16(%eax), %esi #ESI
    movl 20(%eax), %edi #EDI
    movl 28(%eax), %ebp #EBP
    pushl %eax
    movl 36(%eax), %eax #EFLAGS
    pushl %eax
    popfl
    popl %eax
    movl 24(%eax), %esp #ESP
    pushl %eax
    movl 40(%eax), %eax #CR3
    movl %eax, %cr3
    popl %eax
    pushl %eax
    movl 32(%eax), %eax #EIP # TODO: lue devlog, tämä kohta on ainakin oikein!!!
    xchgl (%esp), %eax #We do not have any more registers to use as tmp storage
    movl (%eax), %eax #EAX
    ret #This ends all!
