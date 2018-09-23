.section .text
.global context_switch
.global save_registers

# brief:   save current register values to struct
# @param1: ptr to struct of registers
save_registers:
    mov 44(%esp), %eax # TODO verify this is correct
    mov %ebx, 4(%eax)
    mov %ecx, 8(%eax)
    mov %edx, 12(%eax)
    mov %esi, 16(%eax)
    mov %edi, 20(%eax)
    mov 36(%esp), %ebx
    mov 40(%esp), %ecx
    mov 20(%esp), %edx
    add $4, %edx
    mov 16(%esp), %esi
    mov 4(%esp), %edi
    mov %ebx, (%eax)
    mov %edx, 24(%eax)
    mov %esi, 28(%eax)
    mov %ecx, 32(%eax)
    mov %edi, 36(%eax)
    ret

# @brief:  very similar to task switch but processes
#          are switched, the cr3 reg is updated
# @param1: ptr to struct of registers (current process)
# @param2: ptr to struct of registers (next process)
#
# 1) save state of current process to reg struct
# 2) read state of next process from reg struct to registers
context_switch:
	pusha
    pushf
    mov %cr3, %eax #Push CR3
    push %eax

	# save data of old task to param1
    mov 44(%esp), %eax
    mov %ebx, 4(%eax)
    mov %ecx, 8(%eax)
    mov %edx, 12(%eax)
    mov %esi, 16(%eax)
    mov %edi, 20(%eax)
    mov 36(%esp), %ebx
    mov 40(%esp), %ecx
    mov 20(%esp), %edx
    add $4, %edx
    mov 16(%esp), %esi
    mov 4(%esp), %edi
    mov %ebx, (%eax)
    mov %edx, 24(%eax)
    mov %esi, 28(%eax)
    mov %ecx, 32(%eax)
    mov %edi, 36(%eax)
    pop %ebx
    mov %ebx, 40(%eax)
    push %ebx

	# load data from param2 to registers
    mov 48(%esp), %eax
    mov 4(%eax), %ebx
    mov 8(%eax), %ecx
    mov 12(%eax), %edx
    mov 16(%eax), %esi
    mov 20(%eax), %edi
    mov 28(%eax), %ebp
    push %eax
    mov 36(%eax), %eax
    push %eax
    popf
    pop %eax
    mov 24(%eax), %esp
    push %eax
    mov 40(%eax), %eax
    mov %eax, %cr3
    pop %eax
    push %eax
    mov 32(%eax), %eax
    xchg (%esp), %eax # eip
    mov (%eax), %eax
    ret
