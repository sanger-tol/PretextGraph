/*
Copyright (c) 2026 Genome Research Ltd.

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
#include "utils.h"
#include "file_type.h"
#include "bigwig.h"
#include "gzip.h"
#include <map>

#define String_(x) #x
#define String(x) String_(x)
#define PretextGraph_Version "PretextGraph Version " String(PV)

global_variable
u08
Licence[] = R"lic(
Copyright (c) 2026 Genome Research Ltd.

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

    bigWigToBedGraph (UCSC Genome Browser, Jim Kent)
        https://genome.ucsc.edu/license/
        The build downloads UCSC's bigWigToBedGraph and runs it to convert
        .bigwig and .bw files. That program is covered by the UCSC Genome
        Browser license.

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

const char* debug_bedgraph_file = nullptr;  // used to debug the bedgraph file

u32 NUM_THREADS;
void set_num_threads() { // function to set the number of threads
                         // something interesting happened
                         // if set the num_threads as 1, the thread will be blocked while reading the file into line buffer
                         // TODO solve the thread block problem 
    #ifdef DEBUG
        NUM_THREADS = 4;  // after testing, if using 4 threads, the thread is blocked, haven't found the reason yet
        PrintStatus("Debug mode, number of thread: %d", NUM_THREADS);
    #else
        NUM_THREADS = 4;  // define the thread used in RELEASE mode
        PrintStatus("Release mode, number of thread: %d", NUM_THREADS);
    #endif // DEBUG
    return ;
}


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

struct graph_debug
{   
public:
    f32 *values;
    u32 len;
    u32 dim;
    graph_debug(u32 len_, u32 dim_):len(len_), dim(dim_)
    {
        values = new f32[len * dim];
    }

    ~graph_debug()
    {
        delete[] values;
    }

    f32& operator()(const u32& i, const u32& j)
    {
        return values[i * dim + j];
    }
};

graph_debug* Graph_debug = nullptr;


// the gap extension is different with the coverage and repeat density extensions
// thus we have to add this flag to distinguish the gap extension with the other extensions
// if there is "gap" in the extension name, then the flag will be set as true
// and during the ProcessLine function, the gap value will not be weighted according to the length of the bin and pixel

// add a dictionary to store the data type
unsigned int data_type(0);
std::unordered_map<std::string, int> data_type_dic{  // use this data_type 
    {"default", 0 },
    {"repeat_density", 1},  
    {"gap", 2}, 
    {"coverage", 3},
    {"coverage_avg", 4},
    {"telomere", 5},
    {"not_weighted", 6}
};
std::vector<std::string> data_type_name_vec  = {
    "default", 
    "repeat_density", 
    "gap", 
    "coverage", 
    "coverage_avg", 
    "telomere", 
    "not_weighted"
    };

global_variable
graph *
Graph = nullptr;

global_variable
graph_f *
Graph_tmp = nullptr;

// add the mutex here to protect the graph->values while updating the values in multi-thread mode
// before updating the graph->value calculation into f32, Ed used the atomic operation to update the values
std::mutex mtx_global;  // define the glabal mutex to protext graph_f->values while updating the values

global_variable
volatile u08
Data_Added = 0;

// Set only when the fourth column is a non-negative integer, so a numeric
// bedgraph still saves a single extension under the -n name.
global_variable
volatile u08
Integer_Data_Added = 0;

// Fourth-column labels share the -n track. Each label is one positive value.
std::mutex mtx_groups;
std::unordered_map<std::string, u32> Group_Ids;
f64 *Group_Overlap = 0;

global_variable
volatile u08
Group_Data_Added = 0;

global_function
u32
IsBedgraphLabel(const u08 *text, u32 length)
{
    if (!length || length >= 64) return 0;

    for (u32 i = 0; i < length; ++i)
    {
        u08 c = text[i];
        if (c <= ' ' || c == 127) return 0;
    }

    return 1;
}

global_function
u32
GroupIdFor(const char *label)
{
    std::lock_guard<std::mutex> lock(mtx_groups);

    std::string key(label);
    std::unordered_map<std::string, u32>::iterator found = Group_Ids.find(key);
    if (found != Group_Ids.end()) return found->second;

    u32 id = (u32)Group_Ids.size() + 1;
    Group_Ids.emplace(key, id);
    return id;
}

global_variable
threadSig
Number_of_Lines_Read = 0;

#define Number_of_Lines_Print_Status_Every_Log2 14

global_function
void
ProcessLine(void *in)
{
    u32 sizeGraphArray = Map_Properties->textureResolution * Map_Properties->numberOfTextures1D;
    f64 bp_per_pixel = ((f64)Map_Properties->totalGenomeLength / (f64)sizeGraphArray);  // NOTE: error of 0.0.7 is using u64 type of bp_per_pixel here which will cause the bp_per_pixel a little bit smaller than the real value. Thus the bp_overlap_within_this_pixel will gradually be 0 when it goes to the end of the genome.

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

            f64 bp_left_in_this_bin = (f64) (to_genome - from_genome);

            u32 value;
            if (StringToInt_Check(line + 1, &value, '\n')) // get the value of the bedgraph bin
            {   
                Data_Added = 1;
                Integer_Data_Added = 1;
                u32 from_pixel = (u32)((f64)from_genome / bp_per_pixel); // coordinate with unit of pixel
                u32 to_pixel =   (u32)((f64)to_genome   / bp_per_pixel);

                f64 bp_left_in_current_pixel = (f64)(from_pixel + 1) * bp_per_pixel - (f64)from_genome;
                if (bp_left_in_current_pixel < 0)
                {
                    printf("Error: bp_left_in_current_pixel(%f) < 0. \n", bp_left_in_current_pixel);
                    Assert(0);
                }

                // if (from_pixel != to_pixel) {
                //     printf("from_pixel: %d, to_pixel: %d\n", from_pixel, to_pixel);
                // }

                for (   u32 pixel_id = from_pixel;
                        pixel_id <= to_pixel && pixel_id < sizeGraphArray;
                        ++pixel_id )
                {
                    /* 
                        Iterate over all the pixels covered by the current bin.

                        First calculate the number of bp of the first pixel that the current bin can cover, then add the value to graph->values.
                        Then update the number of bp's left in the current bin, and the number of bp's the current bin can cover.
                    */

                    f64 bp_overlap_within_this_pixel = std::min(bp_left_in_current_pixel, bp_left_in_this_bin); 
                    if (data_type == data_type_dic["gap"]) {  
                        // add the value to graph->values
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[pixel_id] = Min(Max(0.f, (f32)value + Graph_tmp->values[pixel_id]), 1.0f); // if set the value vector as f32 array, then we can not use the atomic operation. If mutil-thread is used, then we have to use the mutex to protect the values
                        lock.unlock();
                    }
                    else if (data_type == data_type_dic["repeat_density"]) // normalise the repeat density data by the length of the bin
                    { 
                        f32 valueToAdd_f = (f32)value * (f32)bp_overlap_within_this_pixel / (f32)bp_per_pixel; 
                        
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[pixel_id] += valueToAdd_f; 
                        lock.unlock();
                    }
                    else if (data_type == data_type_dic["not_weighted"]) //https://github.com/sanger-tol/PretextGraph/issues/3
                    {
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[pixel_id] += value; // if set the value vector as f32 array, then we can not use the atomic operation. If mutil-thread is used, then we have to use the mutex to protect the values
                        lock.unlock();
                    }
                    else  // default data type, just add the value (averaged by bp_per_pixel) to the graph
                    {
                        f32 valueToAdd_f = (f32)value * (f32)bp_overlap_within_this_pixel / (f32)bp_per_pixel; 
                        // add the value to graph->values
                        std::unique_lock<std::mutex> lock(mtx_global);
                        Graph_tmp->values[pixel_id] += valueToAdd_f; // if set the value vector as f32 array, then we can not use the atomic operation. If mutil-thread is used, then we have to use the mutex to protect the values
                        lock.unlock();

                    }
                    #ifdef DEBUG
                        (*Graph_debug)(pixel_id, 0) = Graph_tmp->values[pixel_id]; // value at the pixel
                        (*Graph_debug)(pixel_id, 1) = value;  // original value without processing
                        (*Graph_debug)(pixel_id, 2) = bp_overlap_within_this_pixel;
                        (*Graph_debug)(pixel_id, 3) = bp_per_pixel;
                    #endif // DEBUG
                    // if (valueToAdd < 0) valueToAdd = s32_max;

                    // s32 oldValue = __atomic_fetch_add(Graph->values + index, valueToAdd, 0);
                    
                    // if ((s32_max - oldValue) < valueToAdd)  // if the value is overflow, set this as maximum value (s32_max)
                    // {
                    //     s32 cap = s32_max;
                    //     __atomic_store(Graph->values + index, &cap, 0);
                    // }

                    bp_left_in_this_bin -= bp_overlap_within_this_pixel;
                    bp_left_in_current_pixel = bp_per_pixel;
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
                // Fourth column is a label. All labels share the -n track.
                // The pixel stores the group id, so PretextView colours by group.
                u32 fieldEnd = 0;
                while (line[++fieldEnd] != '\n') {}

                u32 start = 1;
                u32 end = fieldEnd;
                while (start < end && (line[start] == ' ' || line[start] == '\t')) ++start;
                while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r')) --end;

                u32 labelLength = end - start;
                char label[64];
                if (IsBedgraphLabel(line + start, labelLength))
                {
                    for (u32 i = 0; i < labelLength; ++i) label[i] = (char)line[start + i];
                    label[labelLength] = 0;

                    Data_Added = 1;
                    Group_Data_Added = 1;
                    u32 groupId = GroupIdFor(label);
                    u32 from_pixel = (u32)((f64)from_genome / bp_per_pixel);
                    u32 to_pixel = (u32)((f64)to_genome / bp_per_pixel);

                    f64 bp_left_in_current_pixel = (f64)(from_pixel + 1) * bp_per_pixel - (f64)from_genome;
                    if (bp_left_in_current_pixel < 0)
                    {
                        printf("Error: bp_left_in_current_pixel(%f) < 0. \n", bp_left_in_current_pixel);
                        Assert(0);
                    }

                    {
                        std::unique_lock<std::mutex> lock(mtx_global);
                        for (   u32 pixel_id = from_pixel;
                                pixel_id <= to_pixel && pixel_id < sizeGraphArray;
                                ++pixel_id )
                        {
                            f64 bp_overlap_within_this_pixel = std::min(bp_left_in_current_pixel, bp_left_in_this_bin);
                            if (bp_overlap_within_this_pixel > Group_Overlap[pixel_id])
                            {
                                Group_Overlap[pixel_id] = bp_overlap_within_this_pixel;
                                Graph_tmp->values[pixel_id] = (f32)groupId;
                            }

                            bp_left_in_this_bin -= bp_overlap_within_this_pixel;
                            bp_left_in_current_pixel = bp_per_pixel;
                        }
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
                    line[fieldEnd] = 0;
                    PrintWarning("Invalid bedgraph integer data: '%s'", line + 1);
                    line[fieldEnd] = '\n';
                }
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
#elif defined(PRETEXT_X86)
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
#else
    ForLoop(nLanes * 8)
    {
        dataPtr[index] = (s32)((f32)dataPtr[index] * texelsPerBp);
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

global_variable
read_pool *
Read_Pool = 0;

global_variable
s32
Bedgraph_Child = 0;

enum
{
    TRACK_CHILD_NONE = 0,
    TRACK_CHILD_BIGWIG = 1,
    TRACK_CHILD_GZIP = 2
};

global_variable
u32
Track_Child_Kind = TRACK_CHILD_NONE;

// Bedgraph is read directly. bigWig is converted on the fly by bigWigToBedGraph.
global_function
s32
OpenTrackFile(const char *path)
{
    Bedgraph_Child = 0;
    Track_Child_Kind = TRACK_CHILD_NONE;
    if (!path) return STDIN_FILENO;

    track_file_type fileType = TrackFileTypeFromPath(path);
    if (fileType == TRACK_FILE_GZIP)
    {
        gzip_source source = OpenGzipAsBedgraph(path);
        if (source.openResult == GZIP_PIPE_FAILED)
        {
            PrintError("Could not create a pipe for '%s'", path);
            return -1;
        }
        if (source.openResult == GZIP_FORK_FAILED)
        {
            PrintError("Could not start gzip for '%s'", path);
            return -1;
        }
        if (source.openResult == GZIP_UNSUPPORTED)
        {
            PrintError("gzip input is not supported on Windows ('%s'). Decompress it to bedgraph first.", path);
            return -1;
        }

        Bedgraph_Child = source.childPid;
        Track_Child_Kind = TRACK_CHILD_GZIP;
        PrintStatus("Decompressing bedgraph with gzip: %s", path);
        return source.readFd;
    }

    if (fileType == TRACK_FILE_BIGWIG)
    {
        bigwig_source source = OpenBigWigAsBedgraph(path);
        if (source.openResult == BIGWIG_PIPE_FAILED)
        {
            PrintError("Could not create a pipe for '%s'", path);
            return -1;
        }
        if (source.openResult == BIGWIG_FORK_FAILED)
        {
            PrintError("Could not start bigWigToBedGraph for '%s'", path);
            return -1;
        }
        if (source.openResult == BIGWIG_UNSUPPORTED)
        {
            PrintError("bigWig input is not supported on Windows ('%s'). Convert it to bedgraph first.", path);
            return -1;
        }

        Bedgraph_Child = source.childPid;
        Track_Child_Kind = TRACK_CHILD_BIGWIG;
        PrintStatus("Converting bigWig with bigWigToBedGraph: %s", path);
        return source.readFd;
    }

    s32 handle = open(path, O_RDONLY);
    if (handle < 0) PrintError("Could not open '%s'", path);
    return handle;
}

global_variable
volatile u08
Global_Error_Flag = 0;

global_function
void
FinishTrackRead(read_pool *readPool)
{
    ThreadPoolWait(readPool->pool);
    if (debug_bedgraph_file && readPool->handle >= 0) close(readPool->handle);
    readPool->handle = -1;

    if (Track_Child_Kind == TRACK_CHILD_GZIP)
    {
        u32 childResult = FinishGzipChild(Bedgraph_Child);
        if (childResult == GZIP_CHILD_NOT_FOUND)
        {
            PrintError("gzip was not found in PATH. It is required to read '%s'", debug_bedgraph_file);
            Global_Error_Flag = 1;
        }
        else if (childResult == GZIP_CHILD_FAILED)
        {
            PrintError("gzip failed on '%s'", debug_bedgraph_file);
            Global_Error_Flag = 1;
        }
    }
    else
    {
        u32 childResult = FinishBigWigChild(Bedgraph_Child);
        if (childResult == BIGWIG_CHILD_NOT_FOUND)
        {
            PrintError("UCSC bigWigToBedGraph was not found next to PretextGraph or on PATH. The build downloads it, and it is required to read '%s'", debug_bedgraph_file);
            Global_Error_Flag = 1;
        }
        else if (childResult == BIGWIG_CHILD_FAILED)
        {
            PrintError("bigWigToBedGraph failed on '%s'", debug_bedgraph_file);
            Global_Error_Flag = 1;
        }
    }
    Bedgraph_Child = 0;
    Track_Child_Kind = TRACK_CHILD_NONE;
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

global_function
    void
GrabStdIn()
{
    if (!Read_Pool) Read_Pool = CreateReadPool(&Working_Set);

    s32 handle = OpenTrackFile(debug_bedgraph_file);
    if (handle < 0)
    {
        Global_Error_Flag = 1;
        return;
    }
    Read_Pool->handle = handle;

    line_buffer *buffer = TakeLineBufferFromQueue_Wait(Line_Buffer_Queue);
    u32 bufferPtr = 0;
    read_pool *readPool = Read_Pool;

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
    FinishTrackRead(Read_Pool);
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


void output_graph_tmp()
{   
    std::stringstream filename;
    filename << "data_for_test/graph_tmp_" << data_type_name_vec[data_type] << ".txt";
    PrintStatus("Output the graph_tmp values into file: %s\n", filename.str().c_str());
    std::ofstream output_stream(filename.str());
    for (u32 i = 0; i <  Map_Properties->textureResolution * Map_Properties->numberOfTextures1D; ++i) {
        output_stream 
            << (*Graph_debug)(i, 0) <<  "\t " 
            << (*Graph_debug)(i, 1) <<  "\t " 
            << (*Graph_debug)(i, 2) <<  "\t " 
            << (*Graph_debug)(i, 3) << std::endl;
    }
    output_stream.close();
    return;
}

global_function
int
LabelNameRank(const std::string &name)
{
    if (name.size() >= 2 && (name[0] == 'd' || name[0] == 'D'))
    {
        int rank = 0;
        for (size_t i = 1; i < name.size(); ++i)
        {
            if (name[i] < '0' || name[i] > '9') return 100000;
            rank = rank * 10 + (int)(name[i] - '0');
        }
        return rank;
    }

    if (name == "NA") return 1000000;
    return 100000;
}

global_function
bool
LabelNameLess(const std::string &a, const std::string &b)
{
    int rankA = LabelNameRank(a);
    int rankB = LabelNameRank(b);
    if (rankA != rankB) return rankA < rankB;
    return a < b;
}

// First-seen ids depend on thread order. Rewrite pixels so d1, d2, ... then
// other labels, then NA, are 1, 2, 3, ... in that order.
global_function
void
AssignStableGroupIds(u32 mapResolution)
{
    std::vector<std::string> names;
    names.reserve(Group_Ids.size());
    for (std::unordered_map<std::string, u32>::iterator it = Group_Ids.begin(); it != Group_Ids.end(); ++it)
    {
        names.push_back(it->first);
    }
    std::sort(names.begin(), names.end(), LabelNameLess);

    std::vector<u32> remap(Group_Ids.size() + 1, 0);
    for (u32 i = 0; i < (u32)names.size(); ++i)
    {
        u32 provisional = Group_Ids[names[i]];
        remap[provisional] = i + 1;
        PrintStatus("Group %u: %s", i + 1, names[i].c_str());
    }

    ForLoop(mapResolution)
    {
        u32 provisional = (u32)Graph_tmp->values[index];
        if (provisional) Graph_tmp->values[index] = (f32)remap[provisional];
    }
}

struct
track_input
{
    const char *path;
    u32 nameBuffer[16];
    u08 hasPath;
    u08 hasName;

    track_input() : path(0), hasPath(0), hasName(0)
    {
        memset(nameBuffer, 0, sizeof(nameBuffer));
    }
};

global_function
u32
ParseTrackIndex(const char *arg, char key, u32 *indexOut)
{
    if (arg[0] != '-' || arg[1] != key || arg[2] == 0) return 0;

    u32 value = 0;
    const char *cursor = arg + 2;
    while (*cursor)
    {
        if (*cursor < '0' || *cursor > '9') return 0;
        value = value * 10 + (u32)(*cursor - '0');
        ++cursor;
    }

    *indexOut = value;
    return 1;
}

global_function
void
ApplyGraphNameToDataType(u32 *nameInts)
{
    PrintStatus("Graph name: \'%s\'", (char *)nameInts);

    std::string tmp_string((char *)nameInts);
    std::transform(tmp_string.begin(), tmp_string.end(), tmp_string.begin(), ::tolower);
    if (
        tmp_string.find("not") != std::string::npos &&
        tmp_string.find("weighted") != std::string::npos )
    {
        data_type = data_type_dic["not_weighted"];
    }
    else if (tmp_string.find("gap") != std::string::npos)
    {
        data_type = data_type_dic["gap"];
    }
    else if (
        tmp_string.find("repeat") != std::string::npos &&
        tmp_string.find("density") != std::string::npos)
    {
        data_type = data_type_dic["repeat_density"];
    }
    else if (tmp_string.find("coverage") != std::string::npos)
    {
        if (tmp_string.find("avg") == std::string::npos)
        {
            data_type = data_type_dic["coverage"];
        }
        else
        {
            data_type = data_type_dic["coverage_avg"];
        }
    }
    else if (tmp_string.find("telomere") != std::string::npos)
    {
        data_type = data_type_dic["telomere"];
    }
    else
    {
        data_type = data_type_dic["default"];
    }

    PrintStatus("Extension type: %s (%d).", data_type_name_vec[data_type].c_str(), data_type);
}


MainArgs
{   
    #ifdef DEBUG
        printf("\n");
        for (u32 i = 0; i < argc; i++ )
        {
            printf("argv[%d]: %s\n", i, argv[i]);
        }
        printf("\n");
    #endif // DEBUG


    if (ArgCount == 1 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {   
        printf("Usage is not correct, please check the usage below:\n");
        printf("\n%s\n\n", PretextGraph_Version);
        printf(R"help(  One track, from stdin or a file:
    (...bedgraph format |) PretextGraph -i input.pretext -n "graph name"
                                        [-o output.pretext]
                                        [-nf 0|1] (0: disable noise filter, 1: enable, default: 1)
                                        [-f bedgraph.file]
                                        [--help | -h]
                                        [--licence]
                                        [--thirdparty]
    (< bedgraph format)

  Several tracks in one run. Each -tN file is one track and the matching -nN is its name.
  Do not mix this form with a bare -n or -f.
    PretextGraph -i input.pretext -t1 gap.bedgraph -n1 "gap" -t2 telomere.bedgraph -n2 "telomere" -t3 repeat_density.bigwig -n3 "repeat_density" -t4 alg.bedgraph -n4 "alg" [-o output.pretext]

  -i       input Pretext file, required. Sequence names must match the bedgraph.
  -n/-nN   graph name. Known names (gap, telomere, repeat_density, coverage, coverage_avg, not_weighted) select how values are accumulated. Any other name uses the default weighting and is still saved under that name.
  -tN      track file. .bedgraph is read directly. .gz is decompressed with gzip -dc, which must be on PATH. .bigwig and .bw are converted by UCSC bigWigToBedGraph. The build downloads that program and runs the copy next to PretextGraph, or one found on PATH.
  -o       output Pretext file. If omitted, the graph is appended to the input file.
  -nf      noise filter. Applied only to coverage and repeat_density (default: 1).

  A numeric fourth column must be a non-negative integer and is stored under the -n name.
  A label in that column (d1, d2, d3, d4, d5, d6, NA, ...) is stored in that same -n track.
  Each label is a positive integer, so PretextView colours the intervals by group.
  The mapping is printed as "Group N: label".)help");

        printf("\n\nPretextGraph --licence    <- view software licence\n");
        printf("PretextGraph --thirdparty <- view third party software used\n\n");
        exit(EXIT_SUCCESS);
    }

    set_num_threads();
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
    bool NOISE_FILTER = true;
    std::map<u32, track_input> Tracks;

    u32 index_arg = 1;
    while (index_arg < (u32) ArgCount) {   
        std::string arg_tmp(ArgBuffer[index_arg]);
        if (arg_tmp == "-i") {
            ++index_arg;
            if (index_arg >= (u32) ArgCount) {
                PrintError("Input file required for key \'-i\'");
                returnCode = EXIT_FAILURE;
                goto end;
            }
            pretextFile = ArgBuffer[index_arg];
        }
        else if (arg_tmp == "-n") {
            ++index_arg;
            if (index_arg >= (u32) ArgCount) {
                PrintError("Extension name required for key \'-n\'");
                returnCode = EXIT_FAILURE;
                goto end;
            }
            PushStringIntoIntArray(nameBuffer, ArrayCount(nameBuffer), (u08 *)ArgBuffer[index_arg]);
        }
        else if ( arg_tmp == "-o") {
            ++index_arg;
            if (index_arg >= (u32) ArgCount) {
                PrintError("Ouput file path required for key \'-o\'");
                returnCode = EXIT_FAILURE;
                goto end;
            }
            outputFile = ArgBuffer[index_arg];
        }
        else if (arg_tmp == "-f")
        {
            ++index_arg;
            if (index_arg >= (u32) ArgCount) {
                PrintError("bedgraph file path required for key \'-f\'");
                returnCode = EXIT_FAILURE;
                goto end;
            }
            debug_bedgraph_file = ArgBuffer[index_arg];
        }
        else if (arg_tmp == "-nf")
        {
            ++index_arg;
            if (index_arg >= (u32) ArgCount) {
                PrintError("Noise filter flag required for key \'-n\'");
                returnCode = EXIT_FAILURE;
                goto end;
            }
            if (std::string(ArgBuffer[index_arg]) == "0")
            {
                NOISE_FILTER = false;
            }
            else if (std::string(ArgBuffer[index_arg]) == "1")
            {
                NOISE_FILTER = true;
            }
            else
            {
                PrintError("Invalid noise filter flag: (%s), 0: disable NOISE FILTER, 1: enable NOISE FILTER, (default: 1).\n", ArgBuffer[index_arg]);
                returnCode = EXIT_FAILURE;
                goto end;
            }
        }
        else if (arg_tmp.size() > 2 && arg_tmp[1] == 't')
        {
            u32 trackId = 0;
            if (!ParseTrackIndex(arg_tmp.c_str(), 't', &trackId))
            {
                PrintError("Expected -tN file, for example -t1 gap.bedgraph (got '%s')", ArgBuffer[index_arg]);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            ++index_arg;
            if (index_arg >= (u32)ArgCount || ArgBuffer[index_arg][0] == '-')
            {
                PrintError("File path required for -t%u", trackId);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            if (Tracks[trackId].hasPath)
            {
                PrintError("Duplicate -t%u", trackId);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            Tracks[trackId].path = ArgBuffer[index_arg];
            Tracks[trackId].hasPath = 1;
        }
        else if (arg_tmp.size() > 2 && arg_tmp[1] == 'n')
        {
            u32 trackId = 0;
            if (!ParseTrackIndex(arg_tmp.c_str(), 'n', &trackId))
            {
                PrintError("Expected -nN name, for example -n1 \"gap\" (got '%s')", ArgBuffer[index_arg]);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            ++index_arg;
            if (index_arg >= (u32)ArgCount || ArgBuffer[index_arg][0] == '-')
            {
                PrintError("Graph name required for -n%u", trackId);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            if (Tracks[trackId].hasName)
            {
                PrintError("Duplicate -n%u", trackId);
                returnCode = EXIT_FAILURE;
                goto end;
            }
            PushStringIntoIntArray(Tracks[trackId].nameBuffer, ArrayCount(Tracks[trackId].nameBuffer), (u08 *)ArgBuffer[index_arg]);
            Tracks[trackId].hasName = 1;
        }
        index_arg ++ ;
    }

    if (!Tracks.empty() && (nameBuffer[0] || debug_bedgraph_file))
    {
        PrintError("Use either -n with one bedgraph, or -tN/-nN pairs");
        returnCode = EXIT_FAILURE;
        goto end;
    }

    for (std::map<u32, track_input>::iterator trackIt = Tracks.begin(); trackIt != Tracks.end(); ++trackIt)
    {
        if (!trackIt->second.hasPath || !trackIt->second.hasName)
        {
            PrintError("Track %u needs both -t%u file and -n%u name", trackIt->first, trackIt->first, trackIt->first);
            returnCode = EXIT_FAILURE;
            goto end;
        }
    }

        if (!Tracks.empty())
        {
            fprintf(stdout, "[PretextGraph status] :: %u track(s)\n", (u32)Tracks.size());
        }
        else if (debug_bedgraph_file) 
        {
            fprintf(stdout, "[PretextGraph status] :: bedgraph file: %s\n", debug_bedgraph_file);
        }
        else 
        {
            fprintf(stdout, "[PretextGraph status] :: bedgraph file: not specified, read from STDIN_FILENO\n");
        }

    if (pretextFile) {
        fprintf(stdout, "[PretextGraph status] :: Pretext file: %s\n", pretextFile); 
        // PrintStatus("Input file: \'%s\' \n", pretextFile);  // if the path of the file is too long, there can be segmentation fault
    }
    else
    {
        PrintError("Pretext file required");
        returnCode = EXIT_FAILURE;
        goto end;
    }

    if (NOISE_FILTER)
    {
        PrintStatus("Noise filter is enabled");
    }
    else 
    {
        PrintStatus("Noise filter is disabled");
    }

    if (Tracks.empty() && !nameBuffer[0])
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
        // PrintStatus("Output file: \'%s\'\n", outputFile);
        fprintf(stdout, "[PretextGraph status] :: Output Pretext file: %s\n", outputFile); 
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
                        Graph_tmp = new graph_f();
                        Graph_tmp->values = new f32[mapResolution];
                        Group_Overlap = new f64[mapResolution]; 
                        #ifdef DEBUG
                            Graph_debug = new graph_debug(mapResolution, 4);
                        #endif // DEBUG 
                        std::vector<track_input> trackList;
                        if (Tracks.empty())
                        {
                            track_input one;
                            one.path = debug_bedgraph_file;
                            memcpy(one.nameBuffer, nameBuffer, sizeof(nameBuffer));
                            one.hasPath = 1;
                            one.hasName = 1;
                            trackList.push_back(one);
                        }
                        else
                        {
                            for (std::map<u32, track_input>::iterator it = Tracks.begin(); it != Tracks.end(); ++it)
                            {
                                trackList.push_back(it->second);
                            }
                        }

                        for (u32 trackIndex = 0; trackIndex < (u32)trackList.size(); ++trackIndex)
                        {
                            memcpy(nameBuffer, trackList[trackIndex].nameBuffer, sizeof(nameBuffer));
                            ApplyGraphNameToDataType(nameBuffer);
                            debug_bedgraph_file = trackList[trackIndex].path;
                            if (debug_bedgraph_file)
                            {
                                PrintStatus("Reading track file: %s", debug_bedgraph_file);
                            }
                            else
                            {
                                PrintStatus("Reading track from stdin");
                            }

                            memset(Graph_tmp->values, 0, sizeof(f32) * mapResolution);
                            memset(Group_Overlap, 0, sizeof(f64) * mapResolution);
                            memset((void *)Graph->values, 0, sizeof(s32) * mapResolution);
                            Data_Added = 0;
                            Integer_Data_Added = 0;
                            Group_Data_Added = 0;
                            Group_Ids.clear();
                            Number_of_Lines_Read = 0;

                            ThreadPoolAddTask(Thread_Pool, GrabStdIn, 0);
                            ThreadPoolWait(Thread_Pool);

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

                        #ifdef DEBUG
                            output_graph_tmp(); // for check
                        #endif

                        if (Group_Data_Added) AssignStableGroupIds(mapResolution);

                        if (Integer_Data_Added || Group_Data_Added)
                        {
                        PrintStatus("Transfer f32 to s32...");
                        {   

                            // Noise filter. Group ids must stay intact, so skip it for a label track.
                            if (
                                NOISE_FILTER &&
                                !Group_Data_Added &&
                                (
                                    data_type == data_type_dic["coverage"] ||
                                    data_type == data_type_dic["repeat_density"])
                            ) 
                            {   
                                PrintStatus("Noise filter: applied.");
                                f32 medium_value = find_medium(Graph_tmp->values, mapResolution);
                                apply_noise_filter_vec(Graph_tmp->values, mapResolution, medium_value);
                            }

                            // as only the s32 values can be accepted by PretextView
                            ForLoop(mapResolution)
                            {   
                                if ((s32)Graph_tmp->values[index] > std::numeric_limits<s32>::max()) 
                                {   
                                    fprintf(stderr, 
                                        "Error: Graph_tmp->values[%d] = (%f) > s32_max(%d)\n", index, Graph_tmp->values[index], std::numeric_limits<s32>::max());
                                    Assert(0);
                                }
                                Graph->values[index] = (s32)(Graph_tmp->values[index]);
                            }
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
                            FreeLastPush(Working_Set); // comp buffer, so the next track can allocate again
                        }
                        } // numeric values, or one track of fourth-column groups

                        }

                        delete[] Graph_tmp->values;
                        delete Graph_tmp;
                        Graph_tmp = 0;
                        delete[] Group_Overlap;
                        Group_Overlap = 0;
                        #ifdef DEBUG
                            if (Graph_debug) delete Graph_debug;
                        #endif
                        PrintStatus("Done\n\n");
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
