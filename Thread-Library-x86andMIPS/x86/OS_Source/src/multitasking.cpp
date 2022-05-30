
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char* str);
void printfHex(uint8_t);

/*   Task Manager Implementation   */
TaskManager::TaskManager()
{
    this -> testVar = 0;
    this -> thread_size = 0;
    this -> threadIDCounter = 0;
    this -> current_thread_idx = -1;
    this -> firstInterrupt = true;
}

TaskManager::~TaskManager() {}

bool TaskManager::AddThread(Thread* thread)
{
    if (thread -> state == RUNNABLE) {
        return false;
    }

    if (thread_size >= 256)
        return false;

    thread -> ID = ++(this -> threadIDCounter);
    thread -> state = RUNNABLE;
    threads[thread_size++] = thread;
    return true;
}

bool TaskManager::RemoveThread(Thread* thread)
{
    if (this -> thread_size == 0) {
        return false;
    }

    int idx = -1;
    for (int i = 0; i < thread_size; ++i) {
        if (threads[i] -> ID == thread -> ID) {
            idx = i;
        }
    }

    if (idx == -1) {
        return false;
    }

    threads[idx] -> state = DEAD;
    for (int i = idx; i < thread_size - 1; ++i) {
        threads[i] = threads[i + 1];
    }

    --thread_size;
    return true;
}

bool TaskManager::RemoveCurrentThread()
{
    if (this -> thread_size == 0) {
        return false;
    }

    threads[current_thread_idx] -> state = DEAD;
    for (int i = current_thread_idx; i < thread_size - 1; ++i) {
        threads[i] = threads[i + 1];
    }
    --thread_size;
    --current_thread_idx;
    return true;
}

int TaskManager::getTestVar()
{
    return this -> testVar;
}

CPUState* TaskManager::Schedule(CPUState* cpustate, bool unhandled)
{
    // Round Robin Scheduling for context switching
    // My test variable it is dummy variable for test
    this -> testVar += 1;

    // keep first empty cpu state 
    if ((this -> current_thread_idx == -1) && (this -> firstInterrupt)) {
        *(this -> empty_cpustate) = *cpustate;
        this -> firstInterrupt = false;
    }

    // If the thread size zero then load empty cpu to cpu
    if (thread_size <= 0) {
        return this -> empty_cpustate;
    }

    // in the starting current_thread_idx is -1
    // it is no require backup segments
    if ((current_thread_idx >= 0) && (!unhandled)) {

        // backup thread segments
        threads[current_thread_idx] -> cpustate = cpustate;

        // If the thread is not dead then yield it
        threads[current_thread_idx] -> YieldThread();
    }

    // Clean Dead Threads
    int bound = this -> thread_size;
    int tempCurr = this -> current_thread_idx;
    for (int i = 0; i < bound; ++i) {
        if (threads[i] -> state == DEAD) {
            this -> current_thread_idx = i;
            this -> RemoveCurrentThread();
        }
    }

    // If the thread size 0 then load empty into cpu
    if (thread_size == 0) {
        return this -> empty_cpustate;
    }

    // Switching thread
    this -> current_thread_idx = tempCurr;
    if (++current_thread_idx >= thread_size)
        current_thread_idx %= thread_size;

    threads[current_thread_idx] -> state = RUNNING;
    return threads[current_thread_idx] -> cpustate;
}
/*   Task Manager Implementation   */

/*   My Thread Class Implementation */  
Thread::Thread() { this -> state = NEW; }

Thread::~Thread() {}

bool Thread::CreateThread(GlobalDescriptorTable *gdt, TaskManager* taskManager, void entrypoint())
{
    this -> taskManager = taskManager;
    this -> ID = -1;

    // Any task's stack size is 4096 KiB then cpustate must be point to address of (stack + 4096 - sizeof(CPUState))
    // Because all tasks registers keep different values and these register values fragmented by operating system
    this -> cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    this -> cpustate -> eax = 0;
    this -> cpustate -> ebx = 0;
    this -> cpustate -> ecx = 0;
    this -> cpustate -> edx = 0;
    this -> cpustate -> esi = 0;
    this -> cpustate -> edi = 0;
    this -> cpustate -> ebp = 0;
    
    // Security level registers
    //this -> cpustate -> gs = 0;
    //this -> cpustate -> fs = 0;
    //this -> cpustate -> es = 0;
    //this -> cpustate -> ds = 0;
    //this -> cpustate -> error = 0;    
    //this -> cpustate -> esp = ;

    // *entry point* is function address as label address in assembly
    this -> cpustate -> eip = (uint32_t)entrypoint;
    this -> cpustate -> cs = gdt->CodeSegmentSelector();
    //this -> cpustate -> ss = ;
    this -> cpustate -> eflags = 0x202;

    return (this -> taskManager) -> AddThread(this);
}

bool Thread::TermniateThread()
{
    this -> state = DEAD;
    return true;
}

bool Thread::YieldThread()
{
    if (this -> state == DEAD) {
        return false;
    }
    this -> state = RUNNABLE;
    return true;
}

bool Thread::JoinThread()
{
    while (this -> state != DEAD);
    
    return this -> state == DEAD;
}

int Thread::getID()
{
    return this -> ID;
}

THREAD_STATE Thread::getState()
{
    return this -> state;
}
/*   My Thread Class Implementation */