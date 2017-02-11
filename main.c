
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>


//#define DEBUG

/* debug */
#define NUM_OF_ROWS 64
#define NUM_OF_COLUMNS 4

#define DATA_CACHE_ROW 256
#define DATA_CACHE_COLUMN 8
void memory_controller();
typedef struct data_cache_line
{
	bool valid;
	bool full;
	uint32_t tag;
	uint32_t data[8];
	uint32_t offset;
	uint32_t index;
	uint32_t age;
} data_cache_line;
bool hit;
void enqueue(uint32_t address, uint32_t data, bool r_w);
void dequeue(uint32_t address);
bool fill_mc_l2;

data_cache_line dcl [DATA_CACHE_ROW][DATA_CACHE_COLUMN];
int MAXi (uint32_t a, uint32_t b, uint32_t c, uint32_t d);

#define L2_CACHE_ROW 512
#define L2_CACHE_COLUMN 16

#define MISS_DATA_SIZE 8

int stat_cycles;
void cycle()
{
	stat_cycles = stat_cycles + 1;

}
typedef struct l2_cache_line
{
	bool valid;
	bool full;
	uint32_t tag;
	uint32_t data[8];
	uint32_t offset;
	uint32_t index;
	uint32_t age;
	} l2_cache_line;
typedef struct MSHR
{
	bool valid;
	bool done;
	bool full;
	uint32_t address;
	} MSHR;
MSHR mshr [16]={};
bool data_return_mc_l2;
int count_mc_l2;

data_cache_line l2 [L2_CACHE_ROW][L2_CACHE_COLUMN];

#define DRAM_ROW 65536
#define DRAM_COLUMN 256
uint32_t byte_in_block_funct (uint32_t data[8],uint32_t bib);

typedef struct cache_block
{
	uint32_t data[8];
	uint32_t address;
	bool fill;
}
cache_block;


typedef struct cache
{
	uint32_t data[8];
} cache;

//cache dcb[DRAM_ROW][DRAM_COLUMN];

typedef struct bank
{
	  cache dcb [65536][256];
	} bank;

typedef struct rank
{
	bank* bank_var[8];
} rank;
	 // case 0: memcpy(r.bank_var[0].dcb[row_index][column_index].data, miss_data, sizeof(miss_data)); break;

//cache_block* DRAM();
cache_block* DRAM(uint32_t address, uint32_t data, bool r_w);
cache_block my_funct (uint32_t address, uint32_t data, bool r_w);
uint32_t address_mc_l2;
uint32_t data_mc_l2;
bool r_w_mc_l2;


typedef struct requests
{
	uint32_t address;
	uint32_t bank_index;
	uint32_t row_index;
	uint32_t column_index;
	uint32_t data;
	bool r_w;
	bool row_hit,row_miss, closed_row;
	int cycle_of_entry;
	bool valid;
	struct requests *front;
	struct requests *next;
} requests;

struct requests *head = NULL;
struct requests *tail = NULL;


/***************************************************************/
/* Main memory.                                                */
/***************************************************************/

#define MEM_DATA_START  0x10000000
#define MEM_DATA_SIZE   0x00100000
#define MEM_TEXT_START  0x00400000
#define MEM_TEXT_SIZE   0x00100000
#define MEM_STACK_START 0x7ff00000
#define MEM_STACK_SIZE  0x00100000
#define MEM_KDATA_START 0x90000000
#define MEM_KDATA_SIZE  0x00100000
#define MEM_KTEXT_START 0x80000000
#define MEM_KTEXT_SIZE  0x00100000
//#define NUM_OF_ROWS 64
//#define NUM_OF_COLUMNS 4
//cache_line cl [NUM_OF_ROWS][NUM_OF_COLUMNS];

typedef struct {
    uint32_t start, size;
    uint8_t *mem;
} mem_region_t;

/* memory will be dynamically allocated at initialization */
mem_region_t MEM_REGIONS[] = {
    { MEM_TEXT_START, MEM_TEXT_SIZE, NULL },
    { MEM_DATA_START, MEM_DATA_SIZE, NULL },
    { MEM_STACK_START, MEM_STACK_SIZE, NULL },
    { MEM_KDATA_START, MEM_KDATA_SIZE, NULL },
    { MEM_KTEXT_START, MEM_KTEXT_SIZE, NULL }
};

#define MEM_NREGIONS (sizeof(MEM_REGIONS)/sizeof(mem_region_t))



/***************************************************************/
/*                                                             */
/* Procedure: mem_read_32                                      */
/*                                                             */
/* Purpose: Read a 32-bit word from memory                     */
/*                                                             */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
                address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;

            return
                (MEM_REGIONS[i].mem[offset+3] << 24) |
                (MEM_REGIONS[i].mem[offset+2] << 16) |
                (MEM_REGIONS[i].mem[offset+1] <<  8) |
                (MEM_REGIONS[i].mem[offset+0] <<  0);
        }
    }

    return 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: mem_write_32                                     */
/*                                                             */
/* Purpose: Write a 32-bit word to memory                      */
/*                                                             */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
                address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;

            MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
            MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
            MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
            MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
            return;
        }
    }
}
/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Allocate and zero memoryy                       */
/*                                                             */
/***************************************************************/
void init_memory() {
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        MEM_REGIONS[i].mem = malloc(MEM_REGIONS[i].size);
        memset(MEM_REGIONS[i].mem, 0, MEM_REGIONS[i].size);
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename)
{
  FILE * prog;
  int ii, word;

  /* Open program file. */
  prog = fopen(program_filename, "r");
  if (prog == NULL) {
    printf("Error: Can't open program file %s\n", program_filename);
    exit(-1);
  }

  /* Read in the program. */

  ii = 0;
  while (fscanf(prog, "%x\n", &word) != EOF) {
    mem_write_32(MEM_TEXT_START + ii, word);
    ii += 4;
  }

  printf("Read %d words from program into memory.\n\n", ii/4);
}

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files)
{
  int i;

  init_memory();
  //pipe_init();
  for ( i = 0; i < num_prog_files; i++ ) {
    load_program(program_filename);
    while(*program_filename++ != '\0');
  }

  //RUN_BIT = TRUE;
}



uint32_t data_cache_read (uint32_t address)
{//implement write
	  printf ("Address: %x\n", address);
	  uint32_t tag,index,offset,byte_in_block;
	  int i,j,max_age,max_age1,max_age2;
	  uint32_t miss_data[8];
	  cache_block test;

	  tag = (address >> 13) & 0x7ffff;
	  index = (address >> 5) & 0xff;
	  offset = (address >>0) & 0x1f;
	  byte_in_block = (address >> 2) & 0x7;


	//printf ("Address: Tag: %x Index: %x Offset: %x\n",tag,index,offset);

	  for (i=0; i<8;i++)
	  {
		if ((dcl[index][i].tag == tag) && (dcl[index][i].valid ==true))
		{
		  hit = true;
		  switch (byte_in_block){ //8 cases
		  case 0: return dcl[index][i].data[0]; break;
		  case 1: return dcl[index][i].data[1]; break;
		  case 2: return dcl[index][i].data[2]; break;
		  case 3: return dcl[index][i].data[3]; break;
		  case 4: return dcl[index][i].data[4]; break;
		  case 5: return dcl[index][i].data[5]; break;
		  case 6: return dcl[index][i].data[6]; break;
		  case 7: return dcl[index][i].data[7]; break;
		  default: return 0; break;
		  }

		}

	  }
	/*
	  //stat_cycles = stat_cycles + 49;
	  miss_data[7] = mem_read_32 (address+28);//need to modify this for memory hierarchy
	  miss_data[6] = mem_read_32 (address+24);
	  miss_data[5] = mem_read_32 (address+20);
	  miss_data[4] = mem_read_32 (address+16);
	  miss_data[3] = mem_read_32 (address+12);
	  miss_data[2] = mem_read_32 (address+8);
	  miss_data[1] = mem_read_32 (address+4);
	  miss_data[0] = mem_read_32 (address+0);*/
		printf ("address %d  %d \n",address,__LINE__);
		//printf (" data: %d", data);


	  test = my_funct (address, 0x00000000, true);

	  memcpy (miss_data, test.data, sizeof(miss_data));
	  hit=false;
	  max_age1 = MAXi (dcl[index][0].age, dcl[index][1].age, dcl[index][2].age, dcl[index][3].age);
	  max_age2 = MAXi (dcl[index][4].age, dcl[index][5].age, dcl[index][6].age, dcl[index][7].age);
	  max_age = MAXi (max_age1,max_age2, 0,0);


	  for ( j=0; j<8; j++)
	  {
		  printf ("MAx age : %d\n", max_age);
		  if ((dcl[index][j].full == false))
		  {
			  dcl[index][j].tag = tag; //here (for mem hierarchy) before replacing in the cache, the block should be copied back to memory
			  dcl[index][j].index = index;
			  dcl[index][j].offset = offset;
			  dcl[index][j].data[0] = miss_data[0];
			  dcl[index][j].data[1] = miss_data[1];
			  dcl[index][j].data[2] = miss_data[2];
			  dcl[index][j].data[3] = miss_data[3];
			  dcl[index][j].data[4] = miss_data[4];
			  dcl[index][j].data[5] = miss_data[5];
			  dcl[index][j].data[6] = miss_data[6];
			  dcl[index][j].data[7] = miss_data[7];
			  dcl[index][j].age = 0;
			  dcl[index][j].valid = true;
			  dcl[index][j].full = true;
			  printf("inside if false\n");
			  break;
		  }
		  else if (j==7)
		  {	  dcl[index][max_age].age++;
			  dcl[index][max_age].tag = tag; //here (for mem hierarchy) before replacing in the cache, the block should be copied back to memory
			  dcl[index][max_age].index = index;
			  dcl[index][max_age].offset = offset;
			  dcl[index][j].data[0] = miss_data[0];
			  dcl[index][j].data[1] = miss_data[1];
			  dcl[index][j].data[2] = miss_data[2];
			  dcl[index][j].data[3] = miss_data[3];
			  dcl[index][j].data[4] = miss_data[4];
			  dcl[index][j].data[5] = miss_data[5];
			  dcl[index][j].data[6] = miss_data[6];
			  dcl[index][j].data[7] = miss_data[7];
			  dcl[index][max_age].age = 0;
			  dcl[index][max_age].valid = true;
			  dcl[index][max_age].full = true;

		  }
		  else dcl[index][j].age++;

	  }

	  switch (byte_in_block)
	  { //8 cases
		  case 0: return miss_data[0]; break;
		  case 1: return miss_data[1]; break;
		  case 2: return miss_data[2]; break;
		  case 3: return miss_data[3]; break;
		  case 4: return miss_data[4]; break;
		  case 5: return miss_data[5]; break;
		  case 6: return miss_data[6]; break;
		  case 7: return miss_data[7]; break;
		  default: return 0; break;
		}

	  return 0;
}
int MAXi (uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{

	if (a>b && a>c && a>d)
		return 0;
	else if (b>a && b>c && b>d)
		return 1;
	else if (c>a && c>b && c>d)
		return 2;
	else if (d>a && d>c && d>b)
			return 3;
	else
		return 0;
}



int main(int argc, char *argv[])
 {

    int j,k,l;
  /* Error Checking */
  if (argc < 2) {
    printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
           argv[0]);
    exit(1);
  }

  printf("MIPS Simulator\n\n");
  uint32_t data[9];
  initialize(argv[1], argc - 1);
  for (l=0; l<16;l++){
		mshr[l].address = 0x00000000;
		mshr[l].full=false;
		mshr[l].valid=false;
		mshr[l].done=false;
	}

  data[0] = data_cache_read (MEM_TEXT_START);
      //printf ("Hit: %x\n",hit);
  //data[1] = data_cache_read (MEM_TEXT_START+4);

      //printf ("Hit: %x\n",hit);
  //data[2] = data_cache_read (MEM_TEXT_START+8);
      //printf ("Hit: %x\n",hit);
  data[4] = data_cache_read (MEM_TEXT_START+32);
  data[5] = data_cache_read (MEM_TEXT_START+40);

  //printf ("Hit: %x\n",hit);

    for (j=0; j<64; j++)
    {
		for(k=0; k<4; k++)
		{
			printf ("ROW: %d COL: %d", j,k);
			printf (" Valid: %d", dcl[j][k].valid);
			printf (" Tag: %x", dcl[j][k].tag);
			printf (" Index: %x", dcl[j][k].index);
			printf (" Offset: %x", dcl[j][k].offset);
			//printf (" Data: %x", dcl[j][k].data);
			printf (" Age: %d\n", dcl[j][k].age);
		}
	}
	//return 1;
    printf ("%x\n",data[5]);
    return 1;
}


cache_block my_funct (uint32_t address, uint32_t data, bool r_w)
{
  cache_block temp_test;
  static int myfun_count = 0,mem_cont =0 ;
  myfun_count++;
  //MSHR mshr [16]={};
  uint32_t tag,index,byte_in_block,byte_in_block_return;
  int i;//max_age,max_age1,max_age2;
  //uint32_t miss_data[8];
  cache_block* block_dram; //l2_cache_block;

  tag = (address >> 14) & 0x3ffff;
  index = (address >> 5) & 0x1ff;
  byte_in_block = (address >> 2) & 0x7;

		printf ("address %d\n",address);
		printf (" data: %d\n", data);


	for (i=0; i<16;i++)
	{


		/* ----------------------for hit and read---------------------------------------*/
		if ((l2[index][i].tag == tag) && (l2[index][i].valid ==true) && (r_w==true))
		{
			printf("Hit\n");
			stat_cycles = stat_cycles + 14;
			memcpy (temp_test.data, l2[index][i].data, sizeof(l2[index][i].data));
			temp_test.address = address;
			temp_test.fill = true;
			return temp_test;
		}
		/* ----------------------for hit and write---------------------------------------*/
		else if ((l2[index][i].tag == tag) && (l2[index][i].valid ==true) && (r_w==false))
		{
			printf("Hit and write\n");
			switch (byte_in_block)
			{
				case 0:
					l2[index][i].data[0]=data;
				case 1:
					l2[index][i].data[1]=data;
				case 2:
					l2[index][i].data[2]=data;
				case 3:
					l2[index][i].data[3]=data;
				case 4:
					l2[index][i].data[4]=data;
				case 5:
					l2[index][i].data[5]=data;
				case 6:
					l2[index][i].data[6]=data;
				case 7:
					l2[index][i].data[7]=data;
			}

			stat_cycles = stat_cycles + 14;
			memcpy (temp_test.data, l2[index][i].data, sizeof(l2[index][i].data));
			temp_test.address = address;
			temp_test.fill = true;

			return temp_test;
		}
	}

	/*-----------------------MISS-------------------------*/
	bool mshr_full;
	int x,y,z;
	uint32_t tag_return,index_return,offset_return;
	printf("Miss\n");

	for (x=0;x<16; x++)
	{
		printf("Value of x is %d\n",x);
		if (mshr[x].address == address)
			break;
		else if (x==15)
		{
			for (y=0; y<16;y++)
			{
				printf("Value of y is %d\n",y);
				if (mshr[y].full == false)
				{
					printf("Value of y in if is %d\n",y);
					mshr[y].address = address;
					mshr[y].full=true;
					mshr[y].valid=true;
					mshr[y].done=true;
					//stat_cycles = stat_cycles + 5;//latency between L2 and memory controller
					printf ("Address DRAM %x\n",address);
					//block_dram->data = DRAM(address, data, true);//--

					//block_dram->address = address;
					//block_dram->fill = true;
					enqueue(address,data,true);

					break;
				}
				else if(y==15)
				{
					//notify L1 cachsizeof(miss_data *MISS_DATA_SIZE)e of no empty MSHR space
					mshr_full =true;
					printf ("mshr full\n");
					//return NULL;
				}
			}
			break;
		}
	}
	int t;
	for (t=0; t<300; t++)
	{
		mem_cont++;
		//printf("fill_mc_l2t is %d\n",fill_mc_l2);
		memory_controller();
		if (fill_mc_l2 == true)
		{
			printf("Break called\n");
			break;
		}
	}
  printf ("if true: %d/n", fill_mc_l2);
  mem_cont =0;
	if (fill_mc_l2 == true)
	{
		block_dram = DRAM(address_mc_l2, data_mc_l2, r_w_mc_l2);
		printf ("Address!!!: %x\n", address_mc_l2);
		data_return_mc_l2 = true;
		count_mc_l2 = 0;
		fill_mc_l2=false;
	}
	count_mc_l2=5;
	if (data_return_mc_l2 == true && count_mc_l2<5)
	{
		stat_cycles=stat_cycles+1;
		count_mc_l2 ++;
		temp_test.address=0;
		printf ("Address!!!: %x\n", address_mc_l2);
		temp_test.fill=false;
		return temp_test;
	}
	else if (count_mc_l2 == 5 && data_return_mc_l2 == true)
	{
		for (z=0; z<16;z++)
		{
			  if (mshr[z].address == block_dram->address)
				{
					mshr[z].full = false;
				}
			  tag_return = (block_dram->address >> 14) & 0x3ffff;
			  index_return = (block_dram->address >> 5) & 0x1ff;
			  offset_return = (block_dram->address >>0) & 0x1f;
			  byte_in_block_return = (block_dram->address >> 2) & 0x7;
			  l2[index][z].tag = tag_return;
			  l2[index][z].valid = true;
			  l2[index][z].full = true; //need to check full for replacing cache blocks
			  l2[index][z].index = index_return;
			  l2[index][z].offset = offset_return;
			  l2[index][z].age = 0;
			  memcpy (l2[index][z].data, block_dram->data, sizeof(l2[index][z].data));
			  memcpy (temp_test.data, l2[index][z].data, sizeof(l2[index][z].data));
			  temp_test.address = block_dram->address;
			  temp_test.fill = true;
			  printf ("returning from l2\n");
			  return temp_test;
		}
	}
	else
	{
		temp_test.address=0;
		//temp_test.data=NULL;
		temp_test.fill=false;

		return temp_test;
	}

}


uint32_t byte_in_block_funct (uint32_t data[8],uint32_t bib){
switch (bib){ //8 cases
      case 0:  return data[0];
      case 1:  return data[1];
      case 2:  return data[2];
      case 3:  return data[3];
      case 4:  return data[4];
      case 5:  return data[5];
      case 6:  return data[6];
      case 7:  return data[7];
      default: return 0x0;
    }
}

//32---31:16 row index---7:5 bank index---15:8 col index
cache_block* DRAM(uint32_t address, uint32_t data, bool r_w)
{
	cache_block* temp;
	rank *r;
	uint32_t miss_data[MISS_DATA_SIZE];
	uint32_t row_index, column_index, bank_index, byte_in_block;

	printf ("Address in DRAM %x\n",address);

	temp = (cache_block*)(malloc(sizeof(cache_block)));
	if(NULL == temp)
	{
		printf("Memory allocation for cache block failed\n");
		return temp;
	}

	r = (rank *)malloc(sizeof(rank));
	if(NULL == temp)
	{
		printf("Memory allocation for rankfailed\n");
		return temp;
	}

	row_index = (uint32_t)(address >> 16) & 0xffff;
	column_index = (uint32_t)(address >> 8) & 0xff;
	bank_index = (uint32_t)(address >>5) & 0x3;
	byte_in_block = (uint32_t)(address >> 2) & 0x7;


	miss_data[7] = mem_read_32 (address+28);	//need to modify this for memory hierarchy
	miss_data[6] = mem_read_32 (address+24);
	miss_data[5] = mem_read_32 (address+20);
	miss_data[4] = mem_read_32 (address+16);
	miss_data[3] = mem_read_32 (address+12);
	miss_data[2] = mem_read_32 (address+8);
	miss_data[1] = mem_read_32 (address+4);
	miss_data[0] = mem_read_32 (address+0);

	//hit=false;
	//#if 1
	//r->bank_var[bank_index]->dcb[row_index][column_index] = (cache* )(malloc(sizeof(cache)));

	printf("Bank index value is %d\n",bank_index);
	switch (bank_index)
	{
		case 0:
			memcpy(r->bank_var[0]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 1:
			memcpy(r->bank_var[1]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 2:
			memcpy(r->bank_var[2]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 3:
			memcpy(r->bank_var[3]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 4:
			memcpy(r->bank_var[4]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 5:
			memcpy(r->bank_var[5]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 6:
			memcpy(r->bank_var[6]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;

		case 7:
			memcpy(r->bank_var[7]->dcb[row_index][column_index].data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
			break;
	}

	memcpy(temp->data, miss_data, (MISS_DATA_SIZE * sizeof(miss_data )));
	temp->address = address;
	temp->fill = true;

	printf ("Address in DRAM %x\n", address);
	printf ("Data in DRAm %x\t,%x\t,%x\t,%x\t,%x\t,%x\t,%x\t,%x\t\n", miss_data[7], miss_data[6], miss_data[5], miss_data[4], miss_data[3], miss_data[2], miss_data[1], miss_data[0]);

	return temp;
}
int request_count=0;


void enqueue(uint32_t address, uint32_t data, bool r_w)
{
	requests *temp;
	requests *new_request = (requests*)(malloc(sizeof(requests)));
	new_request->row_index = (uint32_t)(address >> 16) & 0xffff;
	new_request->column_index = (uint32_t)(address >> 8) & 0xff;
	new_request->bank_index = (uint32_t)(address >>5) & 0x3;
	new_request->address = address;
	new_request->data = data;
	new_request->r_w  = r_w;
	new_request->cycle_of_entry=stat_cycles;
    request_count++;

    //printf("bank index is %d\n",new_request->bank_index);
	if (head == NULL && tail == NULL)
	{
		new_request->front = NULL;
		new_request->next = NULL;
		head = new_request;
		tail = new_request;

		return;
	}
	else
	{
		temp=tail;
		tail->next =new_request;
		tail = new_request;
		tail->front = temp;
		return;
	}

}

void dequeue(uint32_t address)
{
	requests *temp;
	requests *check = head;
	int i;
	for (i=1; i<=request_count; i++)
	{
		if (check->address == address)
		{
			if (check==head && check==tail)
			{
				head=NULL;
				tail=NULL;
			}
			else if (check->front==head)
			{
				tail=head;
				}
			else if (check->next==tail)
			{
				head=tail;
			}
			else
			{
				check->next->front=check->front;
				check->front->next=check->next;
			}
			request_count--;
			free (check);
			return;
		    }
		temp=check->next;
		check=temp;
		if (i==request_count)
		{
			//printf("ERROR: No requests found to remove\n");
		}

	return;
	}
}

bool closed_row_buffer[8]={true,true,true,true,true,true,true,true};
int active_row[8];
int DRAM_free[8];

int address_command_bus_busy_cycles[8];
int data_bus_busy_cycles[8];

cache_block* service;


typedef struct data_bus_status
{
	bool busy;
	uint32_t address;
	uint32_t data;
	bool r_w;
	int counter;
}data_bus_status;
data_bus_status dbs;

typedef struct bank_status
{
	uint32_t address;
	uint32_t row_buffer;
    bool row_hit,row_miss,closed_row;
	bool row_buffer_empty;
	bool bank_busy;
	uint32_t data;
	bool r_w;
	uint32_t bank_busy_counter;
} bank_status;
bank_status bs[8];

void counters()
{
	/*----------------For DRAM banks---------------*/
	int i;
	for(i=0;i<8;i++)
	{
		if (bs[i].bank_busy==true)
		{
			bs[i].bank_busy_counter++;
		}
		//printf("bank busy counter is %d\n",bs[i].bank_busy_counter);

	}

	/*----------------For data_bus---------------*/
	if (dbs.busy==true)
	{
		dbs.counter++;
	}
	//printf("dbs.counter %d\n",dbs.counter);

	return;

}



void memory_controller()
{
	static int count =0;
	count++;
	int k1=0;
	printf("Count value is %d\n",count);
	requests *schedulable_request[8];
	requests *temp;

	temp = (requests*)(malloc(sizeof(requests)));
	if(NULL == temp)
	{
		printf("Malloc failed for temp\n");
		return;
	}
	temp = head;

	schedulable_request[0] = (requests*)(malloc(sizeof(requests)));


	schedulable_request[1] = (requests*)(malloc(sizeof(requests)));

	//printf("no requests\n");

	schedulable_request[2] = (requests*)(malloc(sizeof(requests)));
	schedulable_request[3] = (requests*)(malloc(sizeof(requests)));
	schedulable_request[4] = (requests*)(malloc(sizeof(requests)));
	schedulable_request[5] = (requests*)(malloc(sizeof(requests)));
	schedulable_request[6] = (requests*)(malloc(sizeof(requests)));
	schedulable_request[7] = (requests*)(malloc(sizeof(requests)));

	for (k1=0;k1<8;k1++)
	{
		if (schedulable_request[k1] == NULL)
		{
			printf("Malloc failed : %d\n",__LINE__);
			return;
		}
	}

	//printf("no requests\n");
    schedulable_request[0]->valid=false;
    schedulable_request[1]->valid=false;
    schedulable_request[2]->valid=false;
    schedulable_request[3]->valid=false;
    schedulable_request[4]->valid=false;
    schedulable_request[5]->valid=false;
    schedulable_request[6]->valid=false;
    schedulable_request[7]->valid=false;
	//printf("%x\n",head->address);
	stat_cycles =stat_cycles+1;
	int i,j,k;
	bool row_hit_schedulable, row_miss_schedulable, closed_row_buffer_schedulable;
	//q = sizeof(address_command_bus_busy_cycles)/sizeof(int);
	//printf ("%d\n", q);
	//printf ("Stat_cycles: %d\n", stat_cycles);
	/*--------------------checking if command and address buses are free----------------*/
	for (j=0; j<8; j++)
	{
		if (address_command_bus_busy_cycles[j]==0)
		{
			row_hit_schedulable=false;
			row_miss_schedulable=false;
			closed_row_buffer_schedulable=true;
			break;
		}
		else if (!(stat_cycles>address_command_bus_busy_cycles[j]+4 || stat_cycles < address_command_bus_busy_cycles[j]-4))
		{	printf("address command busy %d\n",address_command_bus_busy_cycles[j]);
			row_hit_schedulable=false;
			row_miss_schedulable=false;
			closed_row_buffer_schedulable=false;
			break;
		}
		else if (!(stat_cycles+100>address_command_bus_busy_cycles[j]+4 || stat_cycles+100 < address_command_bus_busy_cycles[j]-4))
		{
			row_miss_schedulable=false;
			closed_row_buffer_schedulable=false;

			break;
		}
		else if (!(stat_cycles+200>address_command_bus_busy_cycles[j]+4 || stat_cycles+200 < address_command_bus_busy_cycles[j]-4))
		{
			row_miss_schedulable=false;

			break;
		}
		else if (j==(sizeof(address_command_bus_busy_cycles)/sizeof(int)))
		{
			row_hit_schedulable=true;
			row_miss_schedulable=true;
			closed_row_buffer_schedulable=true;

			break;
		}
	}
	//printf ("After  first for\n");

	for (k=0; k<sizeof(data_bus_busy_cycles)/sizeof(int); k++)
	{
		if (row_hit_schedulable == true && data_bus_busy_cycles[k]>stat_cycles+100 && data_bus_busy_cycles[k]<stat_cycles+150)
		{
			row_hit_schedulable=false;
		}
		if (row_miss_schedulable==true && data_bus_busy_cycles[k]>stat_cycles+300 && data_bus_busy_cycles[k]<stat_cycles+350)
		{
			row_miss_schedulable=false;
		}
		if (closed_row_buffer_schedulable==true && data_bus_busy_cycles[k]>stat_cycles+200 && data_bus_busy_cycles[k]<stat_cycles+250)
		{
			closed_row_buffer_schedulable=false;
		}
	}

	//printf ("hit,miss,closed: %d\t,%d\t,%d\n", row_hit_schedulable, row_miss_schedulable, closed_row_buffer_schedulable);
		//printf ("After  second for\n");

/*--------------------At this point, we will know whether which of the 3 row buffer cases can be implemented using the common buses---------------------*/

/*----------------------Checking whether DRAM is free------------------------------------*/
		bool row_hit[8],row_miss[8],closed_row[8];//---
		bool temp1,temp2;

	int t;
	for (t=0; t<8;t++)
	{
		if (!(stat_cycles > DRAM_free[t] && stat_cycles<DRAM_free[t]+100) || DRAM_free[t]==0 )
		{
			row_hit[t] = row_hit_schedulable;
			temp1=true;
		}
		else
		{
			row_hit[t] = false;
			temp1=false;
		}

		if (!(stat_cycles+100 > DRAM_free[t] || stat_cycles<DRAM_free[t]+200) || DRAM_free[t]==0)
		{
			closed_row[t] = temp1 & closed_row_buffer_schedulable;
			temp2 = true;
		}
		else
		{
			closed_row[t] = false;
			temp2 = false;
		}

		if (!(stat_cycles+200 > DRAM_free[t] || stat_cycles<DRAM_free[t]+300) || DRAM_free[t]==0)
		{
			row_miss[t] = temp2 & row_miss_schedulable;
		}
		else
		{
			row_miss[t] = false;
		}
	}


	/*---------------schedulable request----------*/
	for (i=0; i<request_count; i++)
	{
		if (temp == NULL)
		{
			printf("no requests %d\n",__LINE__);
			break;
		}

		if (closed_row_buffer[temp->bank_index]==true && closed_row[temp->bank_index]==true)
		{
			closed_row_buffer[temp->bank_index] = false;
			schedulable_request[temp->bank_index] = temp;
			schedulable_request[temp->bank_index]->valid = true;
			schedulable_request[temp->bank_index]->row_hit = false;
			schedulable_request[temp->bank_index]->row_miss = false;
			schedulable_request[temp->bank_index]->closed_row = true;
		}
		else if (active_row[temp->bank_index]== temp->row_index && row_hit[temp->bank_index]==true)
		{
			if (schedulable_request[temp->bank_index]->row_index!= temp->row_index)
			{
					schedulable_request[temp->bank_index] = temp;
					schedulable_request[temp->bank_index]->row_hit = true;
					schedulable_request[temp->bank_index]->row_miss = false;
					schedulable_request[temp->bank_index]->closed_row = false;
					schedulable_request[temp->bank_index]->valid = true;

			}
			else
			{
				if ((schedulable_request[temp->bank_index]->cycle_of_entry)<(temp->cycle_of_entry))
				{
					schedulable_request[temp->bank_index] = temp;
					schedulable_request[temp->bank_index]->row_hit = true;
					schedulable_request[temp->bank_index]->row_miss = false;
					schedulable_request[temp->bank_index]->closed_row = false;
					schedulable_request[temp->bank_index]->valid = true;

				}
			}
		}
		else if (row_miss[temp->bank_index]==true)
		{
				schedulable_request[temp->bank_index] = temp;
				schedulable_request[temp->bank_index]->row_hit = false;
				schedulable_request[temp->bank_index]->row_miss = true;
				schedulable_request[temp->bank_index]->closed_row = false;
							schedulable_request[temp->bank_index]->valid = true;

		}

			//check for every scheduled request and update the bank status

	}
			//printf ("schedulabe req: %d\n", schedulable_request[0]->valid);


	int m;
	for (m=0;m<8;m++)
	{
		if (schedulable_request[m]->valid==true)
		{

			//add to the elements of different arrays
			bs[m].address = schedulable_request[m]->address;
			bs[m].data = schedulable_request[m]->data;
			bs[m].r_w = schedulable_request[m]->r_w;
			bs[m].bank_busy=true;
			bs[m].row_hit = schedulable_request[m]->row_hit;
			bs[m].row_miss = schedulable_request[m]->row_miss;
			bs[m].closed_row = schedulable_request[m]->closed_row;

			bs[m].bank_busy_counter = 0;
			bs[m].row_buffer = (uint32_t)(schedulable_request[m]->address >> 16) & 0xffff;
			dequeue(schedulable_request[m]->address);
			schedulable_request[m]->valid=false;
				printf ("inside true\n");
				printf ("bank index %d\n", m);

		}

			//printf ("bank busy: %d\n", bs[0].bank_busy_counter);


	/*--------------initiating bank busy counters---------------*/

		if (bs[m].bank_busy_counter==100 && bs[m].row_hit==true)
		{
			bs[m].bank_busy_counter = 0;
			bs[m].bank_busy = false;
			data_bus_busy_cycles[m]=stat_cycles;
			dbs.busy=true;
			dbs.counter=0;
			dbs.address = bs[m].address;
			dbs.data = bs[m].data;
			dbs.r_w = bs[m].r_w;
		}
		else if (bs[m].bank_busy_counter==200 && bs[m].closed_row==true)
		{
			bs[m].bank_busy_counter = 0;
			bs[m].bank_busy = false;
			data_bus_busy_cycles[m]=stat_cycles;
			dbs.busy=true;
			dbs.counter=0;
			dbs.address = bs[m].address;
			dbs.data = bs[m].data;
			dbs.r_w = bs[m].r_w;
			printf ("double inside: %x\n", bs[m].address);
		}
		else if (bs[m].bank_busy_counter==300 && bs[m].row_miss==true)
		{
			bs[m].bank_busy_counter = 0;
			bs[m].bank_busy = false;
			data_bus_busy_cycles[m]=stat_cycles;
			dbs.busy=true;
			dbs.counter=0;
			dbs.address = bs[m].address;
			dbs.data = bs[m].data;
			dbs.r_w = bs[m].r_w;
		}

	}

	counters();

	if ((dbs.busy==true) && (dbs.counter==50))
	{
		address_mc_l2 = dbs.address;
		data_mc_l2 = dbs.data;
		r_w_mc_l2 = dbs.r_w;
		fill_mc_l2 = true;
		dbs.busy =false;
		dbs.counter=0;
		printf ("Inside memory\n");
	}
	else
	{
		fill_mc_l2 = false;
	}

	return;

	//mem_ctrl scans to find schedulable request
	//1 request => DRAM
	// multiple schedulable request=>row buffer hits => req. that arrived earlier=> request from mem stage i.e. data cache
}










	//4 commands:
	//ACTIVATE: bank/row address
	//READ/WRITE: column address
	//PRECHARGE: bank address
	// each command uses command & address bus for 4 cycles
	// once DRAM receives command => DRAM busy for 100 cycles
	//100 cycles after read/write DRAM sends 32byte chunk of data over data bus
	//data bus busy for 50 cycles

	/*-------row buffer status-----------*/
	//each DRAM bank has a row buffer
	//depending on the status of a bank`s row buffer, different sequences of commands are issued to serve a memory request to a bank
	// 3 cases: closed row buffer: activate, read/write
	//row buffer hit: read/write ;  row buffer miss: precharge, activate, read/write

	/*---------------schedulable request---------*/
	// mem req schedulable when all of its commands can be issued without any conflicts on command,address and data buses as well as the bank
	//


/*
 1. Memory request held in request queue in Memory controller
 2. Each cycle memory controller scans the request queue for schedulable request, including the request that arrived during current cycle
 3. If only one request, schedule for DRAM access.
 4. Multiple request, prioritize row buffer hit request, earliest request, memory request.

 DRAM Access
 1. Each DRAM bank has row buffer
 2. Closed row buffer: activate (0,1,2,3) dram bank free 0-99; read/write (100,101,102,103) dram bank free 100-199; data bus 200-249
 3. Row buffer hit: read/write (0,1,2,3) dram bank free 00-99 data bus 100-149
 4. Row buffer miss:precharge(0,1,2,3) dram bank free 0-99, activate(100,101,102,103)dram bank free 100-199, read/write dram bank free 200-299 data bus 300-349

 */
