/*
Copyright (c) 2021 Ed Harry, Wellcome Sanger Institute

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Header.h"

#define String_(x) #x
#define String(x) String_(x)
#define PretextGraph_Version "PretextMap Version " String(PV)

global_variable
u08
Licence[] = R"lic(
Copyright (c) 2019 Ed Harry, Wellcome Sanger Institute

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)lic";

global_variable
u08
ThirdParty[] = R"thirdparty(
Third-party software and resources used in this project, along with their respective licenses:

    libdeflate (https://github.com/ebiggers/libdeflate)
        Copyright 2016 Eric Biggers

        Permission is hereby granted, free of charge, to any person
        obtaining a copy of this software and associated documentation files
        (the "Software"), to deal in the Software without restriction,
        including without limitation the rights to use, copy, modify, merge,
        publish, distribute, sublicense, and/or sell copies of the Software,
        and to permit persons to whom the Software is furnished to do so,
        subject to the following conditions:

        The above copyright notice and this permission notice shall be
        included in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
        EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
        MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
        NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
        BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.

    stb_sprintf (https://github.com/nothings/stb/blob/master/stb_sprintf.h)
        ALTERNATIVE B - Public Domain (www.unlicense.org)
        This is free and unencumbered software released into the public domain.
        Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
        software, either in source code form or as a compiled binary, for any purpose, 
        commercial or non-commercial, and by any means.
        In jurisdictions that recognize copyright laws, the author or authors of this 
        software dedicate any and all copyright interest in the software to the public 
        domain. We make this dedication for the benefit of the public at large and to 
        the detriment of our heirs and successors. We intend this dedication to be an 
        overt act of relinquishment in perpetuity of all present and future rights to 
        this software under copyright law.
        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
        AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
        WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
)thirdparty";

#ifdef DEBUG
    u32 NUM_THREADS = 1;  // define the thread used in DEBUG mode
#else
    u32 NUM_THREADS = 4;  // define the thread used in RELEASE mode
#endif // DEBUG

global_variable
u08
Status_Marco_Expression_Sponge = 0;

global_variable
char
Message_Buffer[1024];

#define PrintError(message, ...) \
{ \
stbsp_snprintf(Message_Buffer, 512, message, ##__VA_ARGS__); \
stbsp_snprintf(Message_Buffer + 512, 512, "[PretextGraph error] :: %s\n", Message_Buffer); \
fprintf(stderr, "%s", Message_Buffer + 512); \
} \
Status_Marco_Expression_Sponge = 0

#define PrintStatus(message, ...) \
{ \
stbsp_snprintf(Message_Buffer, 512, message, ##__VA_ARGS__); \
stbsp_snprintf(Message_Buffer + 512, 512, "[PretextGraph status] :: %s\n", Message_Buffer); \
fprintf(stdout, "%s", Message_Buffer + 512); \
} \
Status_Marco_Expression_Sponge = 0

#define PrintWarning(message, ...) \
{ \
stbsp_snprintf(Message_Buffer, 512, message, ##__VA_ARGS__); \
stbsp_snprintf(Message_Buffer + 512, 512, "[PretextGraph warning] :: %s\n", Message_Buffer); \
fprintf(stdout, "%s", Message_Buffer + 512); \
} \
Status_Marco_Expression_Sponge = 0

global_variable
memory_arena
Working_Set;

global_variable
thread_pool *
Thread_Pool;

/*
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#pragma clang diagnostic pop
*/

#ifdef __clang__  // Clang pragma
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wreserved-id-macro"
    #pragma clang diagnostic ignored "-Wsign-conversion"
    #pragma clang diagnostic ignored "-Wcast-align"
    #pragma clang diagnostic ignored "-Wextra-semi-stmt"
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wconditional-uninitialized"
    #pragma clang diagnostic ignored "-Wdouble-promotion"
    #pragma clang diagnostic ignored "-Wpadded"
    #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#elif defined(__GNUC__) // GCC pragma
    #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wreserved-id-macro"
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    #pragma GCC diagnostic ignored "-Wcast-align"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wdouble-promotion"
    #pragma GCC diagnostic ignored "-Wpadded"
    #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

// Include the header
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#ifdef __clang__
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

#include "LineBufferQueue.cpp"

struct
contig
{
    u32 name[16];
    f32 fractionalLength;
    f32 previousCumulativeLength;
};

struct
map_properties
{
    contig *contigs;
    u64 totalGenomeLength;
    u32 numberOfContigs;
    u32 textureResolution;
    u32 numberOfTextures1D;
    u32 pad;
};

global_variable
map_properties *
Map_Properties;

#define Contig_Hash_Table_Size 2609
#define Contig_Hash_Table_Seed 5506195799875623629

global_function
u32
GetHashedContigName(u32 *name, u32 nInts)
{
    return(FastHash32((void *)name, (u64)(nInts * 4), Contig_Hash_Table_Seed) % Contig_Hash_Table_Size);
}

global_function
u32
GetHashedContigName(contig *cont)
{
    return(GetHashedContigName(cont->name, ArrayCount(cont->name)));
}

struct
contig_hash_table_node
{
    u32 index;
    u32 pad;
    contig_hash_table_node *next;
};

global_variable
contig_hash_table_node **
Contig_Hash_Table = 0;

global_function
void
InitiateContigHashTable()
{
    Contig_Hash_Table = PushArray(Working_Set, contig_hash_table_node*, Contig_Hash_Table_Size);
    ForLoop(Contig_Hash_Table_Size)
    {
        *(Contig_Hash_Table + index) = 0;
    }
}

global_function
void
InsertContigIntoHashTable(u32 index, u32 hash)
{
    contig_hash_table_node *node = Contig_Hash_Table[hash];
    contig_hash_table_node *nextNode = node ? node->next : 0;

    while (nextNode)
    {
        node = nextNode;
        nextNode = node->next;
    }

    contig_hash_table_node *newNode = PushStruct(Working_Set, contig_hash_table_node);

    newNode->index = index;
    newNode->next = 0;

    (node ? node->next : Contig_Hash_Table[hash]) = newNode;
}

global_function
u32
ContigHashTableLookup(u32 *name, u32 nameLength, contig **cont)
{
    u32 result = 1;

    contig_hash_table_node *node = Contig_Hash_Table[GetHashedContigName(name, nameLength)];
    if (node)
    {
        while (!AreNullTerminatedStringsEqual(name, (Map_Properties->contigs + node->index)->name, nameLength))
        {
            if (node->next)
            {
                node = node->next;
            }
            else
            {
                result = 0;
                break;
            }
        }

        if (result)
        {
            *cont = Map_Properties->contigs + node->index;
        }
    }
    else
    {
        result = 0;
    }

    return(result);
}

struct
graph
{
    volatile s32 *values;
};

// used to store the graph values in f32 format
// actually, don't need the graph_f struct, just use the f32 array to store the values
// but leave this here will help the model to be easier to understand and 
// easier to further extend the code
struct
graph_f
{
    f32 *values;
};

// the gap extension is different with the coverage and repeat density extensions
// thus we have to add this flag to distinguish the gap extension with the other extensions
// if there is "gap" in the extension name, then the flag will be set as true
// and during the ProcessLine function, the gap value will not be weighted according to the length of the bin and pixel
bool name_of_extension_is_gap = false;
global_variable
graph *
Graph;

global_variable
graph_f *
Graph_tmp;

// add the mutex here to protect the graph->values while updating the values in multi-thread mode
// before updating the graph->value calculation into f32, Ed used the atomic operation to update the values
std::mutex mtx_global;  // define the glabal mutex to protext graph_f->values while updating the values

global_variable
volatile u08
Data_Added = 0;

global_variable
threadSig
Number_of_Lines_Read = 0;

#define Number_of_Lines_Print_Status_Every_Log2 14

global_function
void
ProcessLine(void *in)
{
    u32 sizeGraphArray = Map_Properties->textureResolution * Map_Properties->numberOfTextures1D;
    u64 bp_per_pixel = (u64)((f64)Map_Properties->totalGenomeLength / (f64)sizeGraphArray);  // number of bps per pixel

    line_buffer *buffer = (line_buffer *)in; 
    u08 *line = buffer->line;

    ForLoop(buffer->nLines)
    {
        u32 nameBuff[16];

        // std::string line_str((char *)line);
        // std::string name_chr = line_str.substr(0, line_str.find('\t'));
        // line_str.erase(0, name_chr.size() + 1);

        line = PushStringIntoIntArray(nameBuff, ArrayCount(nameBuff), line, '\t'); // the buffer of line moved to '\t'
        contig *cont = 0;
        if (ContigHashTableLookup(nameBuff, ArrayCount(nameBuff), &cont) && cont) // find the contig and get the contig pointer
        {
            u64 prevLength_genome = (u64)((f64)cont->previousCumulativeLength * (f64)Map_Properties->totalGenomeLength); // get the previous cumulative length of the contig and transfer that into bp

            u32 len = 0;
            while (*++line != '\t') ++len;
            u64 from_genome = (u64)StringToInt(line, len) + prevLength_genome;

            len = 0;
            while (*++line != '\t') ++len;
            u64 to_genome = (u64)StringToInt(line, len) + prevLength_genome;

            u64 bp_left_in_this_bin = to_genome - from_genome;

            u32 value;
            if (StringToInt_Check(line + 1, &value, '\n')) // get the value of the bedgraph bin
            {
                Data_Added = 1;

                u32 from_pixel = (u32)(((f64)from_genome / (f64)Map_Properties->totalGenomeLength) * (f64)sizeGraphArray); // coordinate with unit of pixel
                u32 to_pixel = (u32)(((f64)to_genome / (f64)Map_Properties->totalGenomeLength) * (f64)sizeGraphArray);

                u64 bp_covered_in_current_pixel = ((u64)(from_pixel + 1) * bp_per_pixel) - from_genome; //  this is the bp covered in the current pixel

                // if (from_pixel != to_pixel) {
                //     printf("from_pixel: %d, to_pixel: %d\n", from_pixel, to_pixel);
                // }

                for (   u32 index = from_pixel;
                        index <= to_pixel && index < sizeGraphArray;
                        ++index )
                {
                    /* 
                        Iterate over all the pixels covered by the current bin.

                        First calculate the number of bp of the first pixel that the current bin can cover, then add the value to graph->values.
                        Then update the number of bp's left in the current bin, and the number of bp's the current bin can cover.
                    */
                    u32 nThisBin = (u32)(Min(bp_covered_in_current_pixel, bp_left_in_this_bin)); 
                    // s32 valueToAdd = (s32)(value * nThisBin);
                    if (name_of_extension_is_gap) {
                        // add the value to graph->values
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[index] = Min(Max(0.f, (f32)value + Graph_tmp->values[index]), 1.0f); // if set the value vector as f32 array, then we can not use the atomic operation. If mutil-thread is used, then we have to use the mutex to protect the values
                        lock.unlock();
                    }
                    else {
                        f32 valueToAdd_f = (f32)value * (f32)nThisBin / (f32)bp_per_pixel;
                        // add the value to graph->values
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[index] = valueToAdd_f; // if set the value vector as f32 array, then we can not use the atomic operation. If mutil-thread is used, then we have to use the mutex to protect the values
                        lock.unlock();
                    }
                    // if (valueToAdd < 0) valueToAdd = s32_max;

                    // s32 oldValue = __atomic_fetch_add(Graph->values + index, valueToAdd, 0);
                    
                    // if ((s32_max - oldValue) < valueToAdd)  // if the value is overflow, set this as maximum value (s32_max)
                    // {
                    //     s32 cap = s32_max;
                    //     __atomic_store(Graph->values + index, &cap, 0);
                    // }

                    bp_left_in_this_bin -= (u64)nThisBin;
                    bp_covered_in_current_pixel = bp_per_pixel;
                }

                if (!(__atomic_add_fetch(&Number_of_Lines_Read, 1, 0) & ((Pow2(Number_of_Lines_Print_Status_Every_Log2)) - 1)))
                {
                    char buff[128];
                    memset((void *)buff, ' ', 80);
                    buff[80] = 0;
                    printf("\r%s", buff);

                    stbsp_snprintf(buff, sizeof(buff), "\r[PretextGraph status] :: %$d bedgraph lines read", Number_of_Lines_Read);
                    fprintf(stdout, "%s", buff);
                    fflush(stdout);
                }
            }
            else
            {
                u32 tmp = 0;
                while (line[++tmp] != '\n') {}
                line[tmp] = 0;
                PrintWarning("Invalid bedgraph integer data: '%s'", line + 1);
                line[tmp] = '\n';
            }
        }
        else
        {
            PrintWarning("Unknown bedgraph sequence: '%s'", (char *)nameBuff);
        }

        while (*line++ != '\n') {}
    }

    AddLineBufferToQueue(Line_Buffer_Queue, buffer);
}

struct
normalise_graph_thread_data
{
    u32 start;
    u32 nLanes;
    u32 mapResolution;
};

global_function
void
NormaliseGraph_Thread(void *in)
{

    /*
        Normalise the graph data.

        Graph->values = Graph->values * (mapResolution / totalGenomeLength)
    
    */
    normalise_graph_thread_data *data = (normalise_graph_thread_data *)in;
    u32 start = data->start;
    u32 nLanes = data->nLanes;
    u32 mapResolution = data->mapResolution;

    f32 texelsPerBp = (f32)((f64)mapResolution / (f64)Map_Properties->totalGenomeLength); // fraction of pixel per bp
    s32 *dataPtr = ((s32 *)Graph->values) + start; // Graph->values with num_pixels s32 values which save the graph data

#ifdef UsingAVX
    __m256 normFactor = _mm256_set1_ps (texelsPerBp);
    ForLoop(nLanes)
    {
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        __m256i intLane = *((__m256i *)dataPtr);
#pragma clang diagnostic pop
        __m256 floatLane = _mm256_cvtepi32_ps (intLane);
        floatLane = _mm256_mul_ps(floatLane, normFactor);
        intLane = _mm256_cvtps_epi32 (floatLane);

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        *(__m256i *)dataPtr = intLane;
#pragma clang diagnostic pop
        dataPtr += 8;
    }
#else
    nLanes *= 2; // 2048
    __m128 normFactor = _mm_set1_ps (texelsPerBp);
    ForLoop(nLanes)  // 4 values will be processed at one iteration
    {
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        __m128i intLane = *((__m128i *)dataPtr);
#pragma clang diagnostic pop
        __m128 floatLane = _mm_cvtepi32_ps (intLane);
        floatLane = _mm_mul_ps(floatLane, normFactor);
        intLane = _mm_cvtps_epi32 (floatLane);

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        *(__m128i *)dataPtr = intLane;
#pragma clang diagnostic pop
        dataPtr += 4; // process 4 * s32
    }
#endif
}

struct
read_buffer
{
    u08 *buffer;
    u64 size;
};

struct
read_pool
{
    thread_pool *pool;
    s32 handle;
    u32 bufferPtr;
    read_buffer *buffers[2];
};

global_function
read_pool *
CreateReadPool(memory_arena *arena)
{
    read_pool *pool = PushStructP(arena, read_pool);
    pool->pool = ThreadPoolInit(arena, 1);

#define ReadBufferSize MegaByte(16)
    pool->bufferPtr = 0;
    pool->buffers[0] = PushStructP(arena, read_buffer);
    pool->buffers[0]->buffer = PushArrayP(arena, u08, ReadBufferSize);
    pool->buffers[0]->size = 0;
    pool->buffers[1] = PushStructP(arena, read_buffer);
    pool->buffers[1]->buffer = PushArrayP(arena, u08, ReadBufferSize);
    pool->buffers[1]->size = 0;

    return(pool);
}

global_function
void
FillReadBuffer(void *in)
{
    read_pool *pool = (read_pool *)in;
    read_buffer *buffer = pool->buffers[pool->bufferPtr];
    buffer->size = (u64)read(pool->handle, buffer->buffer, ReadBufferSize);
}

global_function
read_buffer *
GetNextReadBuffer(read_pool *readPool)
{
    FenceIn(ThreadPoolWait(readPool->pool));
    read_buffer *buffer = readPool->buffers[readPool->bufferPtr];
    readPool->bufferPtr = (readPool->bufferPtr + 1) & 1;
    ThreadPoolAddTask(readPool->pool, FillReadBuffer, readPool);
    return(buffer);
}

global_variable
volatile u08
Global_Error_Flag = 0;

global_function
    void
GrabStdIn()
{
    line_buffer *buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
    u32 bufferPtr = 0;

    read_pool *readPool = CreateReadPool(&Working_Set);
    readPool->handle = STDIN_FILENO; // read from stdin

// old version used to debug
//     readPool->handle =
// // #ifdef DEBUG
// //         open("data_for_test/repeat_density.bedgraph", O_RDONLY);
// // #else
// //     STDIN_FILENO;
// // #endif
//     STDIN_FILENO; // read from stdin while running

    u08 line[KiloByte(16)];
    u32 linePtr = 0;
    u32 numLines = 0;
    u08 lineTooLong = 0;
    read_buffer *readBuffer = GetNextReadBuffer(readPool);  

    do
    {
        readBuffer = GetNextReadBuffer(readPool);
        for (   u64 bufferIndex = 0;
                bufferIndex < readBuffer->size;
                ++bufferIndex )
        {
            u08 character = readBuffer->buffer[bufferIndex];
            line[linePtr++] = character;

            if (character == '\n')
            {
                if (lineTooLong)
                {
                    u08 c = line[128];
                    line[128] = 0;
                    PrintWarning("Line too long (> %u bytes): '%s...'", sizeof(line), (char *)line);
                    line[128] = c;
                }
                
                if ((u64)linePtr > (Line_Buffer_Size - bufferPtr)) // 
                {
                    buffer->nLines = numLines; // number of lines in the buffer
                    ThreadPoolAddTask(Thread_Pool, ProcessLine, buffer);

                    buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
                    numLines = 0;
                    bufferPtr = 0;
                }

                ForLoop(linePtr) buffer->line[bufferPtr++] = line[index];
                ++numLines;

                linePtr = 0;
                lineTooLong = 0;
            }

            if (linePtr == sizeof(line))
            {
                lineTooLong = 1;
                --linePtr;
            }
        }
    } while (readBuffer->size);

    buffer->nLines = numLines;
    ThreadPoolAddTask(Thread_Pool, ProcessLine, buffer);
}

struct
copy_file_data
{
    FILE *inFile;
    FILE *outFile;
    u08 *buffer;
    u32 bufferSize;
    u32 pad;
};

global_function
void
CopyFile(void *in)
{
    copy_file_data *data = (copy_file_data *)in;
    FILE *outFile = data->outFile;
    FILE *inFile = data->inFile;
    u08 *buffer = data->buffer;
    u32 bufferSize = data->bufferSize;

    if (inFile && outFile)
    {

        u32 readBytes;
        do
        {
            readBytes = (u32)fread(buffer, 1, bufferSize, inFile);
            fwrite(buffer, 1, readBytes, outFile);
        } while (readBytes == bufferSize);
    }
}

MainArgs
{
    if (ArgCount == 1)
    {
        printf("\n%s\n\n", PretextGraph_Version);

        printf(R"help(  (...bedgraph format |) PretextGraph -i input.pretext -n "graph name"
                                        (-o output.pretext)
  (< bedgraph format))help");

        printf("\n\nPretextGraph --licence    <- view software licence\n");
        printf("PretextGraph --thirdparty <- view third party software used\n\n");
        exit(EXIT_SUCCESS);
    }

    if (ArgCount == 2)
    {   
        std::string arg1(ArgBuffer[1]);
        if (arg1.find("licen") != std::string::npos)
        {
            printf("%s\n", Licence);
            exit(EXIT_SUCCESS);
        }

        if (arg1.find("third") != std::string::npos)
        {
            printf("%s\n", ThirdParty);
            exit(EXIT_SUCCESS);
        }
    }

    s32 returnCode = EXIT_SUCCESS;
    const char *pretextFile = 0;
    u32 nameBuffer[16] = {};  
    const char *outputFile = 0;
    u08 *copyBuffer = 0;
    FILE *copyInFile = 0;
    FILE *copyOutFile = 0;
    u32 copyBufferSize = KiloByte(256);
    copy_file_data copyFileData;

    for (   u32 index = 1;
            index < (u32)ArgCount;
            ++index )
    {
        if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"-i"))
        {
            ++index;
            pretextFile = ArgBuffer[index];
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"-n"))
        {
            ++index;
            PushStringIntoIntArray(nameBuffer, ArrayCount(nameBuffer), (u08 *)ArgBuffer[index]);
        }
        else if (AreNullTerminatedStringsEqual((u08 *)ArgBuffer[index], (u08 *)"-o"))
        {
            ++index;
            outputFile = ArgBuffer[index];
        }
    }

    if (pretextFile)
    {
        PrintStatus("Input file: \'%s\'", pretextFile);
    }
    else
    {
        PrintError("Pretext file required");
        returnCode = EXIT_FAILURE;
        goto end;
    }

    if (nameBuffer[0])
    {
        PrintStatus("Graph name: \'%s\'", (char *)nameBuffer);

        // update the name_of_extension_is_gap flag
        std::string tmp_string((char *)nameBuffer);
        if (tmp_string.find("gap") != std::string::npos)
        {
            name_of_extension_is_gap = true;
            PrintStatus("The extension of the graph name is gap, set the name_of_extension_is_gap flag into true.");
        }
    }
    else
    {
        PrintError("Name label required");
        returnCode = EXIT_FAILURE;
        goto end;
    }

    CreateMemoryArena(Working_Set, MegaByte(128));
    Thread_Pool = ThreadPoolInit(&Working_Set, NUM_THREADS);

    if (outputFile) // if output file is specified, define the copy buffer. Copy the input file to output file.
    {
        copyBuffer = PushArray(Working_Set, u08, copyBufferSize);
        copyInFile = fopen((const char *)pretextFile, "rb");
        copyOutFile = fopen((const char *)outputFile, "wb");
        PrintStatus("Output file: \'%s\'", outputFile);
    }
    else
    {
        PrintStatus("Appending graph to input file");
    }

    {
        libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
        u08 magic[] = {'p', 's', 't', 'm'};
        FILE *file;
        PrintStatus("Reading file...");
        if ((file = fopen((const char *)pretextFile, "r+b")))
        {
            u08 magicTest[sizeof(magic)];

            u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file);
            if (bytesRead == sizeof(magicTest))
            {
                ForLoop(sizeof(magic))
                {
                    if (magic[index] != magicTest[index])
                    {
                        fclose(file);
                        file = 0;
                        break;
                    }
                }
            }
            else
            {
                fclose(file);
                file = 0;
            }

            if (!file)
            {
                PrintError("Invalid file format. \'%s\' is not a pretext file", pretextFile);
                returnCode = EXIT_FAILURE;
                goto end;
            }

            /* Read file */
            {
                u32 nBytesHeaderComp;
                u32 nBytesHeader;
                fread(&nBytesHeaderComp, 1, 4, file);
                fread(&nBytesHeader, 1, 4, file);
                u08 *header = PushArray(Working_Set, u08, nBytesHeader);
                u08 *compressionBuffer = PushArray(Working_Set, u08, nBytesHeaderComp);

                fread(compressionBuffer, 1, nBytesHeaderComp, file);
                if (!libdeflate_deflate_decompress(decompressor, (const void *)compressionBuffer, nBytesHeaderComp, (void *)header, nBytesHeader, NULL)) // decompress the header
                {   

                    if (outputFile && !(copyInFile && copyOutFile)) // outputfile exists, and not both of copyin and copyout files exist, then give the error
                    {
                        PrintError("Error copying input file");
                        returnCode = EXIT_FAILURE;
                    }

                    if (!outputFile || (copyInFile && copyOutFile))
                    {
                        if (outputFile)
                        {
                            copyFileData.inFile = copyInFile;
                            copyFileData.outFile = copyOutFile;
                            copyFileData.buffer = copyBuffer;
                            copyFileData.bufferSize = copyBufferSize;
                            ThreadPoolAddTask(Thread_Pool, CopyFile, &copyFileData);
                        }

                        FreeLastPush(Working_Set); // comp buffer

                        u64 val64;
                        u08 *ptr = (u08 *)&val64;
                        ForLoop(8)
                        {
                            *ptr++ = *header++;
                        }
                        u64 totalGenomeLength = val64;

                        u32 val32;
                        ptr = (u08 *)&val32;
                        ForLoop(4)
                        {
                            *ptr++ = *header++;
                        }
                        u32 numberOfContigs = val32;

                        contig *contigs = PushArray(Working_Set, contig, (2 * numberOfContigs));
                        f32 cumulativeLength = 0.0f;
                        ForLoop(numberOfContigs)
                        {
                            contig *cont = contigs + index + numberOfContigs;
                            f32 frac;
                            u32 name[16];

                            ptr = (u08 *)&frac;
                            ForLoop2(4)
                            {
                                *ptr++ = *header++;
                            }

                            cont->fractionalLength = frac;
                            cont->previousCumulativeLength = cumulativeLength;
                            cumulativeLength += frac;

                            ptr = (u08 *)name;
                            ForLoop2(64)
                            {
                                *ptr++ = *header++;
                            }

                            ForLoop2(16)
                            {
                                cont->name[index2] = name[index2];
                            }
                        }

                        u08 textureRes = *header++;
                        u08 nTextRes = *header++;

                        u32 textureResolution = Pow2(textureRes);
                        u32 numberOfTextures1D = Pow2(nTextRes);

                        FreeLastPush(Working_Set); // contigs
                        FreeLastPush(Working_Set); // header

                        Map_Properties = PushStruct(Working_Set, map_properties);
                        Map_Properties->contigs = PushArray(Working_Set, contig, numberOfContigs);
                        u08 *source = (u08 *)(contigs + numberOfContigs);
                        u08 *dest = (u08 *)Map_Properties->contigs;

                        ForLoop(numberOfContigs * sizeof(contig))
                        {
                            *dest++ = *source++;
                        }

                        Map_Properties->totalGenomeLength = totalGenomeLength;
                        Map_Properties->numberOfContigs = numberOfContigs;
                        Map_Properties->textureResolution = textureResolution;
                        Map_Properties->numberOfTextures1D = numberOfTextures1D;

                        InitiateContigHashTable();
                        ForLoop(numberOfContigs)
                        {
                            InsertContigIntoHashTable(index, GetHashedContigName(Map_Properties->contigs + index));
                        }

                        PrintStatus("File read");

                        Line_Buffer_Queue = PushStruct(Working_Set, line_buffer_queue);
                        InitialiseLineBufferQueue(&Working_Set, Line_Buffer_Queue);

                        u32 mapResolution = Map_Properties->textureResolution * Map_Properties->numberOfTextures1D;
                        Graph = PushStruct(Working_Set, graph);

                        u32 graphPlusNameBufferSize = (sizeof(f32) * mapResolution) + sizeof(nameBuffer);  // add the f32 also into this buffer
                        u08 *graphPlusNameBuffer = PushArray(Working_Set, u08, graphPlusNameBufferSize, 5);
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
                        Graph->values = (volatile s32 *)(graphPlusNameBuffer + sizeof(nameBuffer)); //PushArray(Working_Set, volatile s32, mapResolution, 5);
                        // Graph->values_f = (volatile s32 *)(graphPlusNameBuffer + sizeof(nameBuffer) + (sizeof(s32) * mapResolution)); //PushArray(Working_Set, volatile s32, mapResolution, 5);
#pragma clang diagnostic pop                      
                        memset((void *)Graph->values, 0, sizeof(s32) * mapResolution); // initalise the graph values to 0
                        // initialise the graph values_f and set values to 0.
                        Graph_tmp = (graph_f *) malloc(sizeof(graph_f));
                        // Graph_tmp->values = (f32 *) malloc(sizeof(f32) * mapResolution);
                        Graph_tmp->values = (f32 *) calloc(mapResolution, sizeof(f32)); // make sure the values are set to 0

                        ThreadPoolAddTask(Thread_Pool, GrabStdIn, 0); // reading the graph data 
                        ThreadPoolWait(Thread_Pool);
                        fprintf(stdout, "\n");

                        if (Global_Error_Flag) exit(EXIT_FAILURE);

                        if (!Data_Added)
                        {
                            PrintError("No valid bedgraph data processed");
                            exit(EXIT_FAILURE);
                        }

                        /*
                        PrintStatus("Normalising graph data...");
                        {
                            normalise_graph_thread_data data[4];
                            data[0].mapResolution = mapResolution;
                            data[1].mapResolution = mapResolution;
                            data[2].mapResolution = mapResolution;
                            data[3].mapResolution = mapResolution;

                            u32 nLanes = (mapResolution + 7) >> 3; // 4096， total number of units to process, 1 unit = 8 pixels
                            u32 halfLanes = nLanes >> 1;           // 2048   half of the total units
                            u32 quaterLanes = halfLanes >> 1;      // 1024   quater of the total units

                            data[0].nLanes = quaterLanes;          // 1024
                            data[0].start = 0;               

                            data[1].nLanes = halfLanes - data[0].nLanes;
                            data[1].start = data[0].nLanes << 3;

                            data[2].nLanes = quaterLanes;
                            data[2].start = (data[0].nLanes + data[1].nLanes) << 3;

                            data[3].nLanes = nLanes - data[2].nLanes - data[1].nLanes - data[0].nLanes;
                            data[3].start = (data[0].nLanes + data[1].nLanes + data[2].nLanes) << 3;

                            ThreadPoolAddTask(Thread_Pool, NormaliseGraph_Thread, (data + 0));
                            ThreadPoolAddTask(Thread_Pool, NormaliseGraph_Thread, (data + 1));
                            ThreadPoolAddTask(Thread_Pool, NormaliseGraph_Thread, (data + 2));
                            ThreadPoolAddTask(Thread_Pool, NormaliseGraph_Thread, (data + 3));
                            ThreadPoolWait(Thread_Pool);
                        }
                        */

                        PrintStatus("Transfer f32 to s32...");
                        {   
                            // as only the s32 values can be accepted by PretextView
                            ForLoop(mapResolution)
                            {
                                Graph->values[index] = (s32)(Graph_tmp->values[index]);
                            }
                            free(Graph_tmp->values);
                            Graph_tmp->values = NULL; 
                            free(Graph_tmp);
                        }

                        PrintStatus("Saving graph...");
                        {
                            libdeflate_compressor *compressor = libdeflate_alloc_compressor(12);

                            FILE *graphOutputFile = outputFile ? copyOutFile : file;
                            fseek(graphOutputFile, 0, SEEK_END);

                            u32 compBufferSize = graphPlusNameBufferSize + 64;
                            u08 *compBuffer = PushArray(Working_Set, u08, compBufferSize);
                            u32 compSize;

                            u08 graphMagic[4] = {'p', 's', 'g', 'h'};
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
                            u32 *namePtr = (u32 *)graphPlusNameBuffer;
#pragma clang diagnostic pop
                            ForLoop(ArrayCount(nameBuffer))
                            {
                                *namePtr++ = nameBuffer[index];
                            }
                            
                            // compress file by libdeflate, and write the compressed data into the file 
                            if (compressor && (compSize = (u32)libdeflate_deflate_compress(compressor, (const void *)graphPlusNameBuffer, graphPlusNameBufferSize, (void *)compBuffer, compBufferSize)))
                            {
                                fwrite(graphMagic, 1, sizeof(graphMagic), graphOutputFile);
                                fwrite(&compSize, 1, sizeof(u32), graphOutputFile);
                                fwrite(compBuffer, 1, compSize, graphOutputFile);
                            }
                            else
                            {
                                PrintError("Could not compress graph data");
                                returnCode = EXIT_FAILURE;
                            }
                        }
                        PrintStatus("Done");
                    }
                }
                else
                {
                    PrintError("Could not decompress header of \'%s\'", pretextFile);
                    returnCode = EXIT_FAILURE;
                }
            }
        }
        else
        {
            PrintError("Could not open \'%s\'", pretextFile);
            returnCode = EXIT_FAILURE;
        }

        if (file) fclose(file);
        if (copyInFile) fclose(copyInFile);
        if (copyOutFile) fclose(copyOutFile);
    }

    if (Thread_Pool)
    {
        ThreadPoolWait(Thread_Pool);
        ThreadPoolDestroy(Thread_Pool);
    }

end:
    return(returnCode);
}
