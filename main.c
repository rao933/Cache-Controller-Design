#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// help variable used anywhere is just a helping variable (iterative, temp variable etc)

#define INT2VOIDP(i) (void*)(uintptr_t)(i)

//////////////////// GLOBALS ////////////////////

#define MAX_NO_LINES 65536
#define CACHE_SIZE 262144
#define MAX_NO_WAYS 16



uint8_t DATA_WIDTH = 32;
uint8_t N_Ways; // number of ways (1 to 16)
uint32_t N_Lines; // number of lines 
uint8_t Burst_Length; // burst length (1 to 8)
uint32_t Block_size; // Block Size ( data width * BL)
uint32_t TAG_VALUE;
uint32_t LINE_VALUE;

int ii;

uint32_t LRU_MATRIX[MAX_NO_WAYS][MAX_NO_LINES];
uint32_t TAG_MATRIX[MAX_NO_WAYS][MAX_NO_LINES];
bool valid_bit[MAX_NO_WAYS][MAX_NO_LINES];
bool dirty_bit[MAX_NO_WAYS][MAX_NO_LINES];

// USED IN FUNCTIONS

uint8_t tag_b;
bool write_back, write_through_allocate, write_through_non_allocate; 
uint8_t line_b; // the line bits calculated

uint32_t line, tag; // this is used when we got the line number from the function

uint32_t Address_in_use;
bool hit_aur_miss;

// Strategies

/*
i. Write Back
ii. Write Through Allocate
iii. Write Through non allocate
*/

//
FILE *fpt;
//char help[3], help1[4], help2[5];

// COUNTERS

uint32_t rmc; // read mem count
uint32_t rbc; //read block count
uint32_t rbhc; // read block hit count
uint32_t rbmc; // read block miss count
uint32_t rbrdc; // read block replace dirty count 
uint32_t rbrc; // read block replace count
//uint32_t wmc; // same as read just for write (below all)
uint32_t wbc; // write block count
uint32_t wbhc; // write block hit count
uint32_t wbmc; // write block miss count
uint32_t wbrdc; // write block replace dirty count
uint32_t wbrc; // write block replace count
uint32_t wtc; // wrtie through count
uint32_t fc, wmc; // flush count

// we need to have function(s) // here are the prototypes

void Read_Memory(uint32_t *, uint32_t); // reads the memory
void Write_Memory(uint32_t *, uint32_t); // write to the memory
void Calculate_Line(uint8_t, uint8_t); // L = S / Data_Width * BL * N 
void Calculate_Block_Size(uint8_t); // get the block size
void Read_Block(uint32_t, uint32_t); // tag and line arguments
void LRU_update(uint32_t, uint32_t); // updates the LRU based on the stategy did in class
uint8_t calculcate_LRU(uint32_t); // Calculates the LRU
uint32_t TAG(uint32_t); // funtion to calculate tag (uses L and B) not as arguments tho
uint32_t LINE(uint32_t); // funtion to calculate line (uses L and B) not as arguments tho
void Write_Block(uint32_t, uint32_t);
bool hit_or_miss(uint32_t, uint32_t);
void CLEAR();
void CLEAR_COUNTERS();
void FLUSH_CASH();
void print();
void create_file();
void clear_arrays(char*);

void Calculate_Block_Size(uint8_t Burst_Length)
{
    Block_size = Burst_Length * (DATA_WIDTH / 8);
}

void Calculate_Line(uint8_t N_Ways, uint8_t Burst_Length)
{
    N_Lines = (CACHE_SIZE / (N_Ways * Burst_Length * (DATA_WIDTH/ 8)));
}

uint32_t LINE(uint32_t address_value)
{   
    uint8_t help;
    help = log2(Block_size) + tag_b;

    LINE_VALUE = (address_value << tag_b) >> help;

    return LINE_VALUE;
}

uint32_t TAG(uint32_t address_value)
{
    // uint8_t help;
    
    uint32_t help;
    help = log2(N_Lines) + log2(Block_size);
    tag_b = DATA_WIDTH - log2(N_Lines) - log2(Block_size);

    // giving a specifc tag value based on the address_value

    TAG_VALUE = address_value >> help; // a unique identifier for the TAG every time based on add
    return TAG_VALUE;
}


void Read_Memory(uint32_t *add, uint32_t bytes) // address + number of bytes
{
    rmc++;    
    uint32_t old_line = -1; // garbage value
    Address_in_use = (uint32_t) add;
    for (int i = 0; i < bytes; i++)
    {
        tag = TAG(Address_in_use);
        line = LINE(Address_in_use);
    

    if (line != old_line)
    {
          Read_Block(tag, line);
          old_line = line;
    }

    Address_in_use++;
    }
}

void Write_Memory(uint32_t *add, uint32_t bytes)
{
     wmc++;
     if (write_through_allocate || write_through_non_allocate)
    {
        wtc++; // write through count
    }
     uint32_t old_line = -1;
     uint32_t address = (uint32_t) add;
     for (int i = 0; i < bytes; i++)
     {
         tag = TAG(address);
         line = LINE(address);
     
     if (old_line != line)
     {
         Write_Block(tag, line);
         old_line = line;
     }
     address++;
     }
}

void Write_Block(uint32_t tag, uint32_t line)
{
    wbc++; // increment the read counter
    uint8_t write_lru_index = 0;
    write_lru_index = calculcate_LRU(line);
   // write_lru_index = calculcate_LRU(line);
    hit_aur_miss = hit_or_miss(tag, line);
    
    if (!hit_aur_miss && (write_back || write_through_allocate))
    {
        wbmc++;
        
       if(write_back || write_through_allocate) // the first miss is always true
        {
            
            if (dirty_bit[write_lru_index][line] == 1) // if dirty
            {
                wbrc++;
                wbrdc++;
                dirty_bit[write_lru_index][line] = 0;
            }

            TAG_MATRIX[write_lru_index][line] = tag; // update the tag
            valid_bit[write_lru_index][line] = 1; // make valid 1 before leaving
            LRU_update(write_lru_index, line); // update the LRU
            
        }
    }
    if (write_back)
    {
        dirty_bit[write_lru_index][line] = 1;
    }
    if (hit_aur_miss)
    {
        wbhc++;
        valid_bit[write_lru_index][line] = 1; // putting 1 to valid and write nothing 
        LRU_update(write_lru_index, line);
    }
    else
    {
        wbmc++;
    }
}

void CLEAR()
{
    for (int i = 0; i < MAX_NO_LINES; i++)
    {
        for (int j = 0; j < MAX_NO_WAYS; j++)
        {
            LRU_MATRIX[j][i] = 0;
            TAG_MATRIX[j][i] = 0;
            dirty_bit[j][i] = 0;
            valid_bit[j][i] = 0;
        }
    }
}
bool hit_or_miss(uint32_t tag, uint32_t line)
{
    hit_aur_miss = false;
    uint32_t help;
     
    for (int i = 0; i < N_Ways; i++)
    {
        help = TAG_MATRIX[i][line];
        if(help == tag)
        {
            hit_aur_miss = true;
            break;
        }
    }
    return hit_aur_miss;
}
uint8_t calculcate_LRU(uint32_t line)
{
    uint8_t help, i, flag, zero = 0, lru_value;
    for (i = 0; i < N_Ways; i++)
    {
         help = LRU_MATRIX[i][line];
        if (LRU_MATRIX[i][line] == N_Ways - 1) //  LRU value
        {
            flag = 1;
            break;
        }
    }
    if (flag)
    {
        lru_value = i;
        return lru_value;
    }
    else 
    return zero;
}

void LRU_update(uint32_t indexx, uint32_t line)
{
    // N is 1 does not make any sense
    int i;
    uint32_t current_lru_value =  LRU_MATRIX[indexx][line]; // store the current value
    // zeroing th current value and adding 1 (age) to the other values
    // current_lru_value = 0;
    LRU_MATRIX[indexx][line] = 0;
    for (i = 0; i < N_Ways; i++)
    {
        if (LRU_MATRIX[i][line] < current_lru_value)
        {
            LRU_MATRIX[i][line] = LRU_MATRIX[i][line] + 1;
        }
    }
    
}
void Read_Block(uint32_t tag, uint32_t line)
{
    uint8_t help_lru_value = 0; // get the LRU 
    rbc++; // increment the count
    // if the block is already in cahe then we update the LRU and update the hit count
    help_lru_value = calculcate_LRU(line);
    if (hit_or_miss(tag, line)) 
    {
        rbhc++; // increase the hit count
        LRU_update(help_lru_value, line);
    }
    else
    {
        rbmc++; // block miss count increases
        
        if (valid_bit[help_lru_value][line] == 0) 
           {
           if (dirty_bit[help_lru_value][line] == 1) // if dirty
           {
               rbrc++;
               rbrdc++;
               valid_bit[help_lru_value][line] = 0;
               dirty_bit[help_lru_value][line] = 0;
           }
        }
    
    TAG_MATRIX[help_lru_value][line] = tag; // updating tag here
    valid_bit[help_lru_value][line] = 1;
    dirty_bit[help_lru_value][line] = 0;
    LRU_update(help_lru_value, line); // update the LRU in every case
    }     
}  

void CLEAR_COUNTERS()
    {
         rmc = 0; // read mem count
         rbc = 0; //read block count
         rbhc = 0; // read block hit count
         rbmc = 0; // read block miss count
         rbrdc = 0; // read block replace dirty count 
         rbrc = 0; // read block replace count
         wmc = 0; // same as read just for write (below all)
         wbc = 0; // write block count
         wbhc = 0; // write block hit count
         wbmc = 0; // write block miss count
         wbrdc = 0; // write block replace dirty count
         wbrc = 0; // write block replace count
         wtc = 0; // write through memory
         fc = 0;
    }
void FLUSH_CASH()
    {
        if (!write_through_non_allocate)
        {
        for (int i = 0; i < N_Ways; i++)
        {
            for (int j = 0; j < N_Lines; j++)
            {
                if (valid_bit[i][j] == 1 || dirty_bit[i][j] == 1)
                {
                    fc++;
                }
            }
        }
        }
    }
void print()
{
    printf("N = %d\n", N_Ways);
    printf("BL = %d\n", Burst_Length);
    
    if (write_back)
    {
        printf("WRITEBACK\n\n");
    }
    if (write_through_allocate)
    {
        printf("WTA\n\n");
    }
    if (write_through_non_allocate)
    {
        printf("WTNA\n\n");
    }
    printf("READ MEMORY COUNT %d \n", rmc);
    printf("READ BLOCK COUNT %d \n",rbc);
    printf("READ BLOCK HIT COUNT %d \n",rbhc);
    printf("READ BLOCK MISS COUNT %d \n",rbmc);
    printf("READ BLOCK REPLACE %d \n",rbrc);
    printf("READ BLOCK DIRTY REPLACE %d \n",rbrdc);
    printf("WRTIE MEMORY COUNT %d \n", wmc);
    printf("WRITE BLOCK COUNT %d \n",wbc);
    printf("WRITE HIT COUNT %d \n",wbhc);
    printf("WRITE MISS COUNT %d \n",wbmc);
    printf("WRITE REPLACE COUNT %d \n",wbrc);
    printf("WRITE DIRTY REPLACE %d \n",wbrdc);
    printf("WRITE THROUGH %d \n", wtc);
    printf("FLUSH COUNTER %d \n\n", fc);
}
void create_file()
{
    fprintf(fpt, "%d,", N_Ways);
    fprintf(fpt, "%d,", Burst_Length);
    if(write_back)
    {
        char help[3] = "WB";
        fprintf(fpt, "%s,", help);
        clear_arrays(help);
    }
    else if (write_through_allocate)
    {
        char help1[4] = "WTA";
        fprintf(fpt, "%s,", help1);
        clear_arrays(help1);
    }
    else
    {
        char help2[5] = "WTNA";
        fprintf(fpt, "%s,", help2);
        clear_arrays(help2);
    }
   // fprintf(fpt,"N_Ways, BL, RMC, RBC, RBHC, RBMC, RBR, RBDR, WMC, WBC, WHT, WMC, WRC, WDC, WTC, FC \n");
    fprintf(fpt, "%d,", rmc);
    fprintf(fpt, "%d,",rbc);
    fprintf(fpt, "%d,",rbhc);
    fprintf(fpt, "%d,",rbmc);
    fprintf(fpt, "%d,",rbrc);
    fprintf(fpt, "%d,",rbrdc);
    fprintf(fpt, "%d,", wmc);
    fprintf(fpt, "%d,",wbc);
    fprintf(fpt, "%d,",wbhc);
    fprintf(fpt, "%d,",wbmc);
    fprintf(fpt, "%d,",wbrc);
    fprintf(fpt, "%d,",wbrdc);
    fprintf(fpt, "%d,", wtc);
    fprintf(fpt, "%d,\n" ,fc);
}
void clear_arrays(char x[])
{

    for (int i = 0; i < strlen(x); i++)
    {
        x[i] = '\0';
    }
}
        

int main()
{
    fpt = fopen("ajdhsfvkhjadvsfka.csv", "w+");
    fprintf(fpt,"N_Ways, BL, STR, RMC, RBC, RBHC, RBMC, RBR, RBDR, WMC, WBC, WHT, WMC, WRC, WDC, WTC, FC \n");
    uint32_t i, array[65536];

    write_back = true;
    write_through_allocate = false;
    write_through_non_allocate = false;

    for (N_Ways = 1; N_Ways <= 16; N_Ways = N_Ways * 2){
        for (Burst_Length = 1; Burst_Length <= 8; Burst_Length = Burst_Length * 2){
                for (ii = 1; ii <= 3; ii++)
                {
                    Calculate_Block_Size(Burst_Length);
                    Calculate_Line(N_Ways, Burst_Length);
                    if(ii == 1)
                    {
                        write_back = true;
                        write_through_allocate = false;
                        write_through_non_allocate = false;
                    }
                    if (ii == 2)
                    {
                        write_back = false;
                        write_through_allocate = true;
                        write_through_non_allocate = false;
                        
                    }
                    if (ii == 3)
                    {
                        write_back = false;
                        write_through_allocate = false;
                        write_through_non_allocate = true;
                        //printf("3");
                    }
                    
                

CLEAR();
CLEAR_COUNTERS();

// test container

    Write_Memory(&i, sizeof(uint32_t));
    for (i = 0; i < 65536; i++)
    {
        Read_Memory(&i, sizeof(i));
        array[i] = 0;
        Write_Memory(&array[i], sizeof(i));
        Write_Memory(&i, sizeof(i));
    }


    FLUSH_CASH();
    print();
    create_file();

       }   
    }
  }
  fclose(fpt);
}




