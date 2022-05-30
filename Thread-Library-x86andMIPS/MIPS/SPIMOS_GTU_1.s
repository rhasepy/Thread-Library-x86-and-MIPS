.data
start_message: .asciiz "Microkernel SPIMOS_GTU_1 started\n"

.text
.globl main

main:

    li $v0, 4
    la $a0, start_message
    syscall

    jal init
    j exit

exit:

    li $v0, 10
    syscall

init:

    li $v0, 0
    li $a0, 1
    syscall

    jr $ra