//James Kerester and Molly Croke
void printChar(char);
void printString(char*);
void readString(char*);
void handleInterrupt21(int, int, int, int);
void readSector(char*, int);
void readFile(char*, char*,  int*);
void executeProgram(char*);
void terminate();
void writeSector(char*, int);
void deleteFile(char*);
void writeFile(char*, char*, int);
int compareFileNames(char*, char*, int, int);
void handleTimerInterrupt(int, int);
void killProcess(int); 

//global variables
int processActive[8];
int processStackPointer[8];
int currentProcess;


main()
{
	int i;
	makeInterrupt21();

	for(i=0; i<8; i++) //initializing process table
	{
		processActive[i] = 0;
	}
	for(i=0; i<8; i++) //initializing process table
	{
		processStackPointer[i] = 0xff00;
	}
	currentProcess = -1; //initializing process table

	interrupt(0x21, 4, "shell", 0, 0); //test
	makeTimerInterrupt();

	while(1);   /*hang up*/ 
}
//debugging function
void printChar(char c)
{
	interrupt(0x10, 0xe*256 + c, 0, 0, 0);
}

void printString(char* chars)
{
	int i=0;
	while(chars[i] != '\0')
	{
		interrupt(0x10, 0xe*256 + chars[i], 0, 0, 0);
		i++;
	} 
}

void readString(char* line)
{
	char keyPress = interrupt(0x16, 0, 0, 0, 0);
	int i=0;
	interrupt(0x10, 0xe*256 + keyPress, 0, 0, 0);//prints out first char

	while(keyPress != 0xd) //0xd is the enter key
	{
		if(keyPress == 0x8 && i>0)//0x8 is the backspace key
		{
		        i--;//this decrements array index so that it won't
			    //save the character deleted by backspace
		}
		else if(keyPress == 0x8)
		{
		//this handles if array index is already at 0
			i=0;
		}
		else
		{
			//saves char to the array if not a backspace
			line[i] = keyPress;
			i++;
		}
		//gets next char
		keyPress = interrupt(0x16, 0, 0, 0, 0);
		//prints next char
		interrupt(0x10, 0xe*256 + keyPress, 0, 0, 0);

		//if next char is backspace, overwrite prior char with space 
		//and backspace again for the screen
		if(keyPress == 0x8)
		{
		interrupt(0x10, 0xe*256 + ' ', 0, 0, 0);
		interrupt(0x10, 0xe*256 + 0x8, 0, 0, 0);
		}
	}
	line[i] = 0xa; //line feed added to end of array
	line[i+1] = 0x0; //"end of string" added to end of array
	interrupt(0x10, 0xe*256 + 0xd, 0, 0, 0);//prints enter on screen
	interrupt(0x10, 0xe*256 + 0xa, 0, 0, 0);//prints new line on screen
}

void handleInterrupt21( int ax, int bx, int cx, int dx)
{
	if(ax == 0){
		printString(bx);
	}else if(ax == 1){
		readString(bx);
	}else if(ax == 2){
		readSector(bx, cx);
	}else if (ax == 3){
		readFile(bx, cx, dx);
	}else if(ax == 4){
		executeProgram(bx);
	}else if(ax == 5){
		terminate();
	}else if(ax == 6){
		writeSector(bx, cx);
	}else if(ax == 7){
		deleteFile(bx);
	}else if(ax == 8){
		writeFile(bx,cx,dx);
	}else if(ax == 9){
		killProcess(bx);
	}else{
		printString("Error, ax should be less than 10");
	}
}

void readSector(char* buffer, int sector)
{
	int track = 0;
	int head = 0;
	int relSector = sector+1;
	interrupt(0x13, 2*256+1, buffer, track*256+relSector,head*256+0x80);
}

void readFile(char* name, char* buffer, int* sectorsRead)
{
	int length = 0;
	int entry;
	int entryCount = 0;
	char dir[512];
	readSector(dir,2);


	while(name[length] != '\0')
	{
		length++;

	}

	for(entry = 0; entry < 512; entry = entry + 32)
	{
		if(compareFileNames(name, dir, length, entryCount) == 1)
		{
			//use for loop to load the file
			int e;
			for(e=entry+6; e < (entry + 32); e++)
			{   //entry now increments by one instead of 32
				if(dir[e] != 0)
				{
					readSector(buffer, dir[e]);
					buffer += 512;
					++*sectorsRead;
					//*sectorsRead = *sectorsRead+1;
				}
				else
				{
					break;
				}
			}
		}
		entryCount++;
	}
}

void executeProgram(char* name)
{
	int freeSpace;
	int segCalc;
	int i;
	int sectorsRead=0;
	char buffer[13312];
	int address;
	readFile(name, buffer, &sectorsRead);

	//this prevents trying to execute a file that doesn't exist
	if(sectorsRead >  0)
        {
		int dataseg = setKernelDataSegment();//protection for global variables
		for(i=0; i<8; i++)//searches through processActive for free spot
		{
			if(processActive[i]==0)
			{
				freeSpace = i;
				break;
			}
		}
		restoreDataSegment(dataseg); //protection for global variables

		segCalc = (freeSpace+2)*0x1000; //determine the segment

		for(address = 0; address < 13312; address++)
		{
			putInMemory(segCalc, address, buffer[address]);
		}
		initializeProgram(segCalc);//put initializeProgram here

		dataseg = setKernelDataSegment();//protection for global variables
		processActive[freeSpace] = 1;
		processStackPointer[freeSpace] = 0xff00;
		restoreDataSegment(dataseg); //protection for global variables
	}
}

void terminate()
{
	int dataseg = setKernelDataSegment();
	processActive[currentProcess] = 0;
	restoreDataSegment(dataseg);

	while(1);
}

void writeSector(char* buffer, int sector)
{
        int track = 0;
        int head = 0;
        int relSector = sector+1;
        interrupt(0x13, 3*256+1, buffer, track*256+relSector,head*256+0x80);
}

void deleteFile(char* name)
{
	int length = 0;
	int entry;
	int entryCount = 0;
	char dir[512];
	char map[512];

	readSector(dir, 2);
	readSector(map, 1);

	  while(name[length] != '\0')
        {
                length++;

        }


	for(entry = 0; entry < 512; entry = entry + 32)
	{
		if(compareFileNames(name, dir, length, entryCount) == 1)
		{
			int e;
			dir[entry] = '\0';

                        for(e=entry+6; e < (entry + 32); e++)
			{
				map[dir[e]] = 0; //= 0??
			}
		}
		entryCount++; 
	}
	writeSector(dir, 2);
	writeSector(map, 1);

}

void writeFile(char* buffer, char* name, int numberOfSectors)
{
	char map[512];
	char directory[512];
	int i;
	int k;
	int x;
	int e;

//load the map and directory sectors
	readSector(map,1);
	readSector(directory,2);
//find a free directory entry and copy the name to that entry 
	for(i = 0; i < 512; i = i+32 )
	{
		if(directory[i] == '\0')
		{
			for(e=0; e<32; e++)
			{
				//fills all 32 bytes with \0 in case file name
				//is less than 6
				directory[i+e] = '\0';
			}
			for(e=0; e<6; e++)
			{
				directory[i+e] = name[e];
			}
			break;
		}
	}

//find a free sector by searching through the map for a 0 starting at 3
//sets that sector to 0xFF in the Map
//Adds that sector number to the file's directory entry
//Writes 512 bytes from the buffer holding the file to that sector
		for(k=3; k<512; k++)
		{
			if(map[k] == 0 && numberOfSectors != 0)
			{
				map[k] = 0xFF;
				directory[i+6] = k;
				writeSector(buffer, k);
				buffer += 512;

				writeSector(map, 1);
				writeSector(directory, 2);

				numberOfSectors--;
				i++; //goes to next byte in directory entry
			}
		}
	return;
}

int compareFileNames(char* file1, char* file2, int length, int entryCount)
{
	int i;
	entryCount = entryCount*32;

        for(i=0; i<length; i++)
        {
                if( file1[i] != file2[entryCount+i])
                {
                        return 0;
                }
        }
        return 1;
}

void handleTimerInterrupt(int segment, int sp)
{
	int i;

	int dataseg = setKernelDataSegment();
	//printChar('T');
        //printChar('i');
        //printChar('c');


	for(i=0; i<8; i++)//draws active process at top right
        {
                putInMemory(0xb800,60*2+i*4,i+0x30);
                if(processActive[i]==1)
                        putInMemory(0xb800,60*2+i*4+1,0x20);
                else
                        putInMemory(0xb800,60*2+i*4+1,0);
        }

	if(currentProcess != -1)
	{
		processStackPointer[currentProcess] = sp;
	}

	while(1)
	{
		currentProcess++;
		if(currentProcess >= 8)
		{
			currentProcess = 0;
		}
		if (processActive[currentProcess] == 1)
		{
			break;
		}
	}
	segment = (currentProcess+2)*0x1000;
	sp = processStackPointer[currentProcess];
	restoreDataSegment(dataseg);

	returnFromTimer(segment, sp);
}

void killProcess(int id)
{
	int dataseg = setKernelDataSegment();
	processActive[id] = 0;
	restoreDataSegment(dataseg);
}
