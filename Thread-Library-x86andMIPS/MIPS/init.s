.data

################ ITEM POOL ################
item1:	                .word	50
item2:	                .word   5
item3:	                .word	4
item4:	                .word   10
item5:	                .word   23
item6:	                .word	6
item7:	                .word	12
item8:	                .word	86
item9:	                .word	77
item10:	                .word   17
item11:                 .word   11
item12:                 .word   41
item13:                 .word   7
item14:                 .word   19
item15:                 .word   14
item16:                 .word   98
item17:                 .word   37
item18:                 .word   76
item19:                 .word   2
item20:                 .word   40
################ ITEM POOL ################

############### ARRAY INFO ################
thread_id_array:        .space 1024
size:	                .word	20
array:
                        .word   item1,
                        .word	item2,
                        .word	item3,
                        .word	item4,
                        .word	item5,
                        .word	item6,
                        .word	item7,
                        .word	item8,
                        .word	item9,
                        .word	item10,
                        .word	item11,
                        .word   item12,
                        .word   item13,
                        .word   item14,
                        .word   item15,
                        .word   item16,
                        .word   item17,
                        .word   item18,
                        .word   item19,
                        .word   item20
############### ARRAY INFO ################

error:                  .asciiz "Microkernel pan!c\n"
kernel1_message:        .asciiz "Microkernel 1: Main Thread running...\n"
kernel2_message:        .asciiz "Microkernel 2: Main Thread running...\n"
kernel1_term_message:   .asciiz "\nMicrokernel 1: Main Thread shut down...\n"
kernel2_term_message:   .asciiz "\nMicrokernel 2: Main Thread shut down...\n"

thread_msg:             .asciiz "Thread\n"
init_thread:            .asciiz "Inıt Thread\n"
producer_msg:           .asciiz "\nProducer Thread Actions: \n"
consumer_msg:           .asciiz "\nConsumer Thread Actions: \n"
init_msg:               .asciiz "\nInıt Process Actions: \n"
produce_msg:            .asciiz "Producer produced "
product:                .asciiz " product\n"
consume_msg:            .asciiz "Consumer consumed "
joined:                 .asciiz "Inıt joined thread 1 (producer)\n"
joined2:                .asciiz "Inıt joined thread 2 (consumer)\n"
general_joined:         .asciiz "Inıt joined thread "
endline:                .asciiz "\n"
endOfProcess:           .asciiz "\nEnd of the MergeSort...\n\n"
sizeMsg:                .asciiz "Enter the array size: "
elemMsg:                .asciiz "Enter element: "
space:                  .asciiz " "
invalid:                .asciiz "\nInvalid input...\n"
oldArr:                 .asciiz "Old Array: "
sortedArr:              .asciiz "Sorted Array: "
for_thread:             .asciiz "\nFor Thread:"
for_main:               .asciiz "\n\nFor Main:"

.text

init_main:
.globl main

########################################
# İnitiliaze Kernel
    addi $t1, $zero, 1
    addi $t2, $zero, 2

    beq $v0, $t1, micro_kernel_1
    beq $v0, $t2, micro_kernel_2
########################################

# ---------------------------------------- #
########################################
# 1th Microkernel (SPIMOS_GTU_1.s)
# Main Thread Microkernel 1
micro_kernel_1:

    li $v0, 4
    la $a0, kernel1_message
    syscall

    # Thread ID array and this array index init
    la $t0, thread_id_array
    add $t3, $t0, $zero

    addi $t1 $zero, 4
    addi $t4, $zero, 1
    loop_create:

        beq $t1, $zero, loop_create_end

        li $v0, 18
        la $a0, thread
        add $a1, $t4, $zero
        syscall
        # Save Thread Id in array to thread join
        sb $v0, 0($t3)

        addi $t1, $t1, -1
        addi $t3, $t3, 4
        addi $t4, $t4, 1
        j loop_create

    loop_create_end:
    
    pthread_join:
        add $t1, $zero, 4

        loop_join:
            beq $t1, $zero, loop_join_end
            
            li $v0, 19
            lb $a0, 0($t0)
            syscall

            li $v0, 4
            la $a0, general_joined
            syscall

            li $v0, 1
            lb $a0, 0($t0)
            syscall

            li $v0, 4
            la $a0, endline
            syscall

            addi $t1, $t1, -1
            addi $t0, $t0, 4
            j loop_join

    loop_join_end:

        addi $t0, $zero, 2 # divider
        addi $sp, $sp, -12 # take stack

        #merge(0, (MAX / 2 - 1) / 2, MAX / 2 - 1);
        add $t1, $zero, $zero
        sw $t1, 0($sp) # param 1 (0)

        add $t1, $zero, $zero
        lw $t1, size
        div $t1, $t0
        mflo $v0
        addi $v0, $v0, -1
        div $v0, $t0
        mflo $v0
        sw $v0, 4($sp) # param 2 ((MAX / 2 - 1) / 2)

        lw $t1, size
        div $t1, $t0
        mflo $v0
        addi $v0, $v0, -1
        sw $v0, 8($sp) # param 3 (MAX / 2 - 1)

        lw $t0, 0($sp)
        lw $t1, 4($sp)
        lw $t2, 8($sp)
        addi $t2, $t2, 1
        sll $t0, $t0, 2
        sll $t1, $t1, 2
        sll $t2, $t2, 2
        la $t3, array
        add $a0, $t3, $t0
        add $a1, $t3, $t1
        add $a2, $t3, $t2
        jal merge

        #merge(MAX / 2, MAX/2 + (MAX-1-MAX/2)/2, MAX - 1);
        lw $t1, size
        div $t1, $t0
        mflo $v0
        sw $v0, 0($sp) # param 1 (MAX / 2)

        lw $t1, size
        div $t1, $t0
        mflo $v0
        addi $t2, $zero, -1
        mult $v0, $t2
        mflo $v0
        addi $v0, $v0, -1
        lw $t1, size
        add $v0, $v0, $t1
        add $t2, $v0, $zero
        div $t1, $t0
        mflo $v0
        add $v0, $v0, $t2
        sw $v0, 4($sp) # param 2 (MAX/2 + (MAX-1-MAX/2)/2)

        lw $t1, size
        addi $t1, $t1, -1
        sw $t1, 8($sp) # param 3 (MAX - 1)

        
        lw $t0, 0($sp)
        lw $t1, 4($sp)
        lw $t2, 8($sp)
        sll $t0, $t0, 2
        sll $t1, $t1, 2
        sll $t2, $t2, 2
        la $t3, array
        add $a0, $t3, $t0
        add $a1, $t3, $t1
        add $a2, $t3, $t2
        jal merge

        #merge(0, (MAX - 1)/2, MAX - 1);
        add $t1, $zero, $zero
        sw $t1, 0($sp) # param 1 (0)

        lw $t1, size
        addi $t1, $t1, -1
        div $t1, $t0
        mflo $v0
        sw $v0, 4($sp) # param 2 ((MAX - 1)/2)

        lw $t1, size
        addi $t1, $t1, -1
        sw $t1, 8($sp) # param 3 (MAX - 1)

        
        lw $t0, 0($sp)
        lw $t1, 4($sp)
        lw $t2, 8($sp)
        sll $t0, $t0, 2
        sll $t1, $t1, 2
        sll $t2, $t2, 2
        la $t3, array
        add $a0, $t3, $t0
        add $a1, $t3, $t1
        add $a2, $t3, $t2
        jal merge

        li $v0, 4
        la $a0, for_thread
        syscall

        j printArr_t

        print_ret:
            li $v0, 4
            la $a0, for_main
            syscall
            
            la $a0, array
            lw $t0, size
            sll $t0, $t0, 2
            add $a1, $a0, $t0
            jal mergesort

            addi $sp, $sp, 12 # bring stack
            
            j printArr_m

        j micro_kernel_1_exit

# Microkernel 1 default thread
    # void* thread(void* arg) -> arg = $a1
    # $a1 is value, that part of array for this thread 
    thread:

        addi $t0, $zero, 4 # divider
        addi $a1, $a1, -1  # thread number - init

        addi $sp, $sp, -16
        # 0($sp) thread_part
        sw $a1, 0($sp) 

        # int low = thread_part * (MAX / 4);
        la $t1, size
        lb $t1, 0($t1)
        div $t1, $t0
        mflo $v0
        mult $v0, $a1
        mflo $v0
        sw $v0, 4($sp)

        # int high = (thread_part + 1) * (MAX / 4) - 1;
        la $t1, size
        lb $t1, 0($t1)
        div $t1, $t0
        mflo $v0
        addi $a1, $a1, 1
        mult $a1, $v0
        mflo $v0
        addi $v0, $v0, -1
        sw $v0, 8($sp)

        # int mid = low + (high - low) / 2;
        lw $t1, 4($sp) # low
        lw $t2, 8($sp) # high

        sub $v0, $t2, $t1
        addi $t0, $zero, 2
        div $v0, $t0
        mflo $v0
        add $v0, $v0, $t1
        sw $v0, 12($sp)

        lw $t0, 4($sp)  # init param low
        lw $t1, 8($sp)  # init param high

        sll $t0, $t0, 2
        sll $t1, $t1, 2
        addi $t1, $t1, 4
        la $t3, array
        add $a0, $t3, $t0
        add $a1, $t3, $t1
        slt $t2, $t0, $t1
        beq $t2, 1, merge_thread

        merge_thread:
            jal mergesort
        
        # Bring stack pointer
        addi $sp, $sp, 16

        li $v0, 24
        syscall

        _dummy:
            j _dummy

# 1th Microkernel's Main Thread terminate function
micro_kernel_1_exit:

    li $v0, 4
	la $a0, endline
	syscall

    li $v0, 4
    la $a0, kernel1_term_message
    syscall

    li $v0, 10
    syscall
########################################
# ---------------------------------------- #

# ---------------------------------------- #
# 2th Microkernel function (SPIMOS_GTU_2.s)
# Main Thread Microkernel 2 
micro_kernel_2:

# Kernel 2 İnitiliaze
    li $v0, 4
    la $a0, kernel2_message
    syscall

    # Thread ID array and this array index init
    la $t0, thread_id_array
    add $t3, $t0, $zero

# Pthread_create syscall for producer thread
    li $v0, 18
    la $a0, producer_thread
    syscall
    # Save Thread Id in array to thread join
    sb $v0, 0($t3)
    addi $t3, $t3, 4

# Pthread_create syscall for consumer thread
    li $v0, 18
    la $a0, consumer_thread
    syscall
    # Save Thread ID in array to thread join
    sb $v0, 0($t3)
    addi $t3, $t3, 4

    pthread_join_2:
        # join thread for thread_array[0]
        li $v0, 19
        lb $a0, 0($t0)
        syscall

        # If joining is true
        # then joined msg printing
        li $v0, 4
        la $a0, general_joined
        syscall

        li $v0, 1
        lb $a0, 0($t0)
        syscall

        li $v0, 4
        la $a0, endline
        syscall

        # join thread for thread_array[1]
        li $v0, 19
        lb $a0, 4($t0)
        syscall

        # IF joining is true
        # then joined msg printing
        li $v0, 4
        la $a0, general_joined
        syscall

        li $v0, 1
        lb $a0, 4($t0)
        syscall

        li $v0, 4
        la $a0, endline
        syscall

        # Exit main thread
        j micro_kernel_2_exit 

# Thread returning
thread_return:
    li $v0, 24
    syscall

    # if the context switch doesn't work,
    # dummy should return until swtich.

    # It can be context switch after return, 
    # but this is against the principle of round robin.
    dummy_:
        j dummy_

# Producer Thread function
    producer_thread:

        addi $a1, $zero, 31
        add $t0, $zero, $zero

        producer_loop:

            li $v0, 4
            la $a0, producer_msg
            syscall

            addi $a1, $a1, -1
            addi $t0, $t0, 1
            beq $a1, $zero, thread_return

            li $v0, 21
            syscall

            li $v0, 4
            la $a0, produce_msg
            syscall

            li $v0, 1
            add $a0, $t0, $zero
            syscall

            li $v0, 4
            la $a0, product
            syscall

            li $v0, 22
            syscall
            
            j producer_loop

# Consumer Thread function
    consumer_thread:

        addi $a1, $zero, 31
        add $t0, $zero, $zero

        consumer_loop:

            li $v0, 4
            la $a0, consumer_msg
            syscall

            addi $a1, $a1, -1
            addi $t0, $t0, 1
            beq $a1, $zero, thread_return

            li $v0, 21
            syscall

            li $v0, 4
            la $a0, consume_msg
            syscall

            li $v0, 1
            add $a0, $t0, $zero
            syscall

            li $v0, 4
            la $a0, product
            syscall
            
            li $v0, 22
            syscall
            
            j consumer_loop
########################################

    j micro_kernel_2_exit

# 2th Microkernel's Main Thread terminate function
micro_kernel_2_exit:

    li $v0, 4
    la $a0, kernel2_term_message
    syscall

    j exit
########################################
# ---------------------------------------- #

# Exit Function
exit:
    li $v0, 23
    syscall
########################################

# void mergeSort(int arr[],int l,int r)
# int l -> $a0
# int r -> $a1
mergesort:
    
    # since the function is recursive, 
    # it is necessary to store the parameters and the return value
	addi $sp, $sp, -16		
	sw $ra, 0($sp)	
	sw $a0, 4($sp)	
	sw $a1, 8($sp)

	sub $t0, $a1, $a0 # if (l >= r) 
	ble	$t0, 4, mergesort_return # return;
    # calculate parameters r and m
	srl	$t0, $t0, 3		 
	sll	$t0, $t0, 2		
	add	$a1, $a0, $t0		
	sw $a1, 12($sp) 
	jal	mergesort # mergesort(arr,l,r)
	
	lw $a0, 12($sp) # load calculated parameters	 
	lw $a1, 8($sp) # load calculated parameters
	jal	mergesort # mergesort(arr, m+1, r)
	
    # load parameters
	lw $a0, 4($sp)
	lw $a1, 12($sp)		
	lw $a2, 8($sp)	    
	jal	merge # merge(arr, l, m, r)

# sort end jump 31th register
mergesort_return:				

	lw $ra, 0($sp) # load return adress
	addi $sp, $sp, 16 # bring strack pointer
	jr $ra # return to linked adress
	

# void merge(int arr[], int l, int m, int r)
# int l -> $a0
# int m -> $a1
# int r -> $a2
merge:
    # since the function is recursive, 
    # it is necessary to store the parameters and the return value
	addi $sp, $sp, -16
	sw $ra, 0($sp)		
	sw $a0, 4($sp)		
	sw $a1, 8($sp)	
	sw $a2, 12($sp)	
	
	move $s0, $a0
	move $s1, $a1
	
mergeloop:

    # load parameter adress
    # and fill L[] and R[]
	lw $t0, 0($s0)		
	lw $t1, 0($s1)	
	lw $t0, 0($t0)	
	lw $t1, 0($t1)	
	
    # i < n1 
	bgt	$t1, $t0, update_jump
	
	move $a0, $s1		
	move $a1, $s0		
	jal	shift_swap
	addi $s1, $s1, 4

update_jump:

	addi $s0, $s0, 4
	lw	$a2, 12($sp)
    # i < n1
	bge	$s0, $a2, mergeloop_out
    # j < n1
	bge	$s1, $a2, mergeloop_out

	j	mergeloop
	
mergeloop_out:

	lw $ra, 0($sp) # load return adress
	addi $sp, $sp, 16 # bring stack pointer
	jr $ra # return adress


# shift(array, param1, param2)
# param1 -> $a0 -> l
# param2 -> $a1 -> r
shift_swap:

	li $t0, 10
	ble	$a0, $a1, shift_return # loop control
	addi $t6, $a0, -4

    # get param1 pointer
	lw $t7, 0($a0)
    # param1 controller 
	lw $t8, 0($t6)
    
    sw $t7, 0($t6)		
	sw $t8, 0($a0)	

	move $a0, $t6
	j shift_swap

shift_return:
	jr $ra # return

# print array for thread
printArr_t:
	li $v0, 4
	la $a0, endline
	syscall
	
	li $v0, 4
	la $a0, sortedArr
	syscall
	
    la $t0, array
    lw $t1, size
    
	addi $t2, $zero, 0
	add $t3, $t0, $zero
	for:
		beq $t2, $t1, print_ret
		
		li $v0, 1
		lw $a0, 0($t3)
        lw $a0, 0($a0)
		syscall
		
		li $v0, 4
		la $a0, space
		syscall
		
		addi $t3, $t3, 4
		addi $t2, $t2, 1
		j for
j print_ret

# print array for main
printArr_m:
	li $v0, 4
	la $a0, endline
	syscall
	
	li $v0, 4
	la $a0, sortedArr
	syscall
	
    la $t0, array
    lw $t1, size
    
	addi $t2, $zero, 0
	add $t3, $t0, $zero
	for_:
		beq $t2, $t1, micro_kernel_1_exit
		
		li $v0, 1
		lw $a0, 0($t3)
        lw $a0, 0($a0)
		syscall
		
		li $v0, 4
		la $a0, space
		syscall
		
		addi $t3, $t3, 4
		addi $t2, $t2, 1
		j for_
j micro_kernel_1_exit

exit_:		
	li $v0, 4
	la $a0, endline
	syscall

    li $v0, 10
    syscall