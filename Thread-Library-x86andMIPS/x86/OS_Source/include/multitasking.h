 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{ 
    enum THREAD_STATE { NEW, RUNNABLE, RUNNING, DEAD };

    class TaskManager;

    // In the CPU registers, the registers value different between task and another task
    // If there is context switch then the registers must be recovered in the task class
    struct CPUState
    {
        // accumulator register
        common::uint32_t eax;
        // base register
        common::uint32_t ebx;
        // counting register
        common::uint32_t ecx;
        // data register
        common::uint32_t edx;

        // stack index
        common::uint32_t esi;
        // data index
        common::uint32_t edi;
        // stack base pointer
        common::uint32_t ebp;

        /*
        // Security level registers
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */

        // error code
        common::uint32_t error;

        // instruction pointer
        common::uint32_t eip;
        // code segment
        common::uint32_t cs;
        // flags
        common::uint32_t eflags;
        // stack pointer
        common::uint32_t esp;
        // stack segment
        common::uint32_t ss; 

    } __attribute__((packed));

    class Thread
    {
        friend class TaskManager;

        private:
            THREAD_STATE state;
            TaskManager* taskManager;
            common::uint8_t stack[4096]; // 4 KiB
            CPUState* cpustate;
            int ID;

        public:
            Thread();
            ~Thread();

            bool CreateThread(GlobalDescriptorTable *gdt, TaskManager* taskManager, void entrypoint());
            bool TermniateThread();
            bool YieldThread();
            bool JoinThread();

            int getID();
            THREAD_STATE getState();
    };
    
    class TaskManager
    {
        private:
            CPUState* empty_cpustate;
            Thread* threads[256];
            int thread_size;
            int current_thread_idx;
            int testVar;
            int threadIDCounter;
            bool firstInterrupt;

        public:
            TaskManager();
            ~TaskManager();
            bool AddThread(Thread* thread);
            bool RemoveThread(Thread* thread);
            bool RemoveCurrentThread();
            int getTestVar();
            CPUState* Schedule(CPUState* cpustate, bool unhandled);
    };
}

#endif