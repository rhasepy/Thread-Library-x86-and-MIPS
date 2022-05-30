
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

//#define CREATE_FINISH_TEST
//#define CREATE_TERMINATE_JOIN_TEST
//#define CREATE_TERMINATE_JOIN_TEST_2
//#define WITH_PETERSON_TEST
//#define WITHOUT_PETERSON_TEST

void printf(char* str);
void printfHex(uint8_t key);
void printfInt(int key, char* lineOpt);
char* itoa(int num, char* str, int base);

GlobalDescriptorTable gdt;
TaskManager taskManager;
InterruptManager interrupts(0x20, &gdt, &taskManager);

Thread MainThread;
Thread ProducerThread;
Thread ConsumerThread;

int testVar = 0;
int index = 0;
int itemSize = 0;

//////// PETERSON SOLUTION BLOCK ////////
#define PRODUCER    0
#define CONSUMER    1
#define SIZE        2

int turn;
int itemCount = 0;
bool thread_flags[SIZE];

void lock_with_peterson(int which)
{
    int other = 1 - which;
    thread_flags[which] = true;
    turn = which;

    while (turn == which && thread_flags[other]);
}

void unlock_with_peterson(int which)
{    
    thread_flags[which] = false;
}
//////// PETERSON SOLUTION BLOCK ////////

void ProduceEvent()
{
    printf("\nProducer Started\n");

    int count = 0;
    while(true) {

        while (count < 999999) { count++; }
        printf(" P");
        count = 0;
    }

    printf("Producer End\n");
}

void ConsumeEvent()
{
    printf("\nConsumer Started\n");

    int count = 0;
    while(true) {

        while (count < 999999) { count++; }
        printf(" C");
        count = 0;
    }

    printf("Consumer End\n");
}

void MainEvent()
{
    ProducerThread.TermniateThread();
    ConsumerThread.TermniateThread();
    ProducerThread.JoinThread();
    printf("\nProducer Joined");
    ConsumerThread.JoinThread();
    printf("\nConsumer Joined");
}

void MainEvent_2()
{
    ProducerThread.TermniateThread();
    ProducerThread.JoinThread();
    printf("\nProducer Joined");
    ConsumerThread.JoinThread();
    printf("\nConsumer Joined");
}

void CreateFinishEvent_1()
{
    printf("\nTest1 Started\n");
    for (int i = 0; i < 7; ++i) {
        printf("T1 ");
    }
    printf("\n");
    printf("Test1 Finished Task...\n");
}

void CreateFinishEvent_2()
{
    printf("\nTest2 Started\n");
    for (int i = 0; i < 7; ++i) {
        printf("T2 ");
    }
    printf("\n");
    printf("Test2 Finished Task...\n");
}

void Producer_Peterson_Event()
{
    lock_with_peterson(PRODUCER);
    printf("\nProducer Start To Produce Item");
    printf("\nAssume that 5 P are one item");
    int count = 0;
    for (int i = 0; i < 10; ++i) {
        while (count < 9999999) { count++; }

        if (i %2 == 0)
            printf("\nP");
        count = 0;
    }
    ++itemSize;
    printf("\nProducer Produced Item");
    unlock_with_peterson(PRODUCER);
}

void Consumer_Peterson_Event()
{
    lock_with_peterson(CONSUMER);
    printf("\nConsumer Start To Consume Item");
    if (itemSize <= 0) {
        printf("\nConsumer Can not Consumed Item (Critical Section)");
    }
    else {
        printf("\nConsumer Consumed Item");
    }
    unlock_with_peterson(CONSUMER);
}

void Producer_NoPeterson_Event()
{
    printf("\nProducer Start To Produce Item");
    printf("\nAssume that 5 P are one item");
    int count = 0;
    for (int i = 0; i < 10; ++i) {
        while (count < 9999999) { count++; }

        if (i %2 == 0)
            printf("\nP");
        count = 0;
    }
    ++itemSize;
    printf("\nProducer Produced Item");
}

void Consumer_NoPeterson_Event()
{
    printf("\nConsumer Start To Consume Item");
    if (itemSize <= 0) {
        printf("\nConsumer Can not Consumed Item (Critical Section)");
    }
    else {
        printf("\nConsumer Consumed Item");
    }
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

// bu method baska bir yerde tanımlanıp burada implement edilmiştir
// data segment farklı bu yüzden main thread şart
extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("171044033 - Homework 1 - MoyOS\n");

    #ifdef CREATE_FINISH_TEST
        printf("Create Finish Test\n");
        // Test simple create thread
        // The thread start and do its task
        Thread test_thread;
        Thread test_thread2;

        test_thread.CreateThread(&gdt, &taskManager, CreateFinishEvent_1);
        test_thread2.CreateThread(&gdt, &taskManager, CreateFinishEvent_2);
    #endif

    #ifdef CREATE_TERMINATE_JOIN_TEST
        printf("Create Terminate Join Test 1\n");
        // Thread variable is global because
        // Main thread should call terminate or join
        // Kernel should not wait
        // Test all of termniate and joining
        ProducerThread.CreateThread(&gdt, &taskManager, ProduceEvent);
        ConsumerThread.CreateThread(&gdt, &taskManager, ConsumeEvent);
        MainThread.CreateThread(&gdt, &taskManager, MainEvent);
    #endif

    #ifdef CREATE_TERMINATE_JOIN_TEST_2
        printf("Create Terminate Join Test 2\n");
        // Test one thread terminate other running
        // Test while running joining thread
        ProducerThread.CreateThread(&gdt, &taskManager, ProduceEvent);
        ConsumerThread.CreateThread(&gdt, &taskManager, ConsumeEvent);
        MainThread.CreateThread(&gdt, &taskManager, MainEvent_2);
    #endif

    #ifdef WITH_PETERSON_TEST
        printf("With Peterson Test\n");
        Thread producer_;
        Thread consumer_;

        producer_.CreateThread(&gdt, &taskManager, Producer_Peterson_Event);
        consumer_.CreateThread(&gdt, &taskManager, Consumer_Peterson_Event);
    #endif

    #ifdef WITHOUT_PETERSON_TEST
        printf("Without Peterson Test\n");
        Thread producer;
        Thread consumer;

        producer.CreateThread(&gdt, &taskManager, Producer_NoPeterson_Event);
        consumer.CreateThread(&gdt, &taskManager, Consumer_NoPeterson_Event);
    #endif

    // If the all of above running then yield is running
    // Because yield is about scheduling if the thread state
    // Changes running to runnable then thread yielt
    // In the round robin scheduling, yield does not work real time

    interrupts.Activate();

    while(1);
}

void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}

void reverse(char str[], int len )
{
    int start, end;
    char temp;
    for(start=0, end=len-1; start < end; start++, end--) {
        temp = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = temp;
    }
}

char* itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

void printfInt(int key, char* lineOpt)
{
    char Buffer[256];
    printf(itoa(key, Buffer, 10));
    printf(lineOpt);
}