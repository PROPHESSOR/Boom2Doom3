// // Header

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

// Patches

#define bool    char
#define true    1
#define false   0

// Constants

#define SIDE_NULL 0xFFFF

#define WALL_WEIGHT 8
#define FLAT_WEIGHT 8

// Structures

typedef struct {
    int16_t x;
    int16_t y;
} vertex_t;

typedef struct {
    uint16_t v1;
    uint16_t v2;
    uint16_t flags;
    uint16_t type;  // Line type
    uint16_t tag;   // Sector tag
    uint16_t s1;    // Right sidedef (0xFFFF = SIDE_NULL = null)
    uint16_t s2;    // Left sidedef   ^
} linedef_t;

typedef struct {
    uint16_t    offset_x;
    uint16_t    offset_y;
    uint8_t     texture_upper[8];
    uint8_t     texture_middle[8];
    uint8_t     texture_lower[8];
    uint16_t    sector;
} sidedef_t;

typedef struct {
    int16_t     height_floor;
    int16_t     height_ceil;
    uint8_t     texture_floor[8];
    uint8_t     texture_ceil[8];
    int16_t     brighness;
    uint16_t    special;
    uint16_t    tag;
} sector_t;

// Functions

int     getFileSize(void);
int     getMinSectorFloor(int);
int     getMaxSectorHeight(int);
int16_t getLinedefAngle(int);

bool    parseVertexes(void);
bool    parseLinedefs(void);
bool    parseSidedefs(void);
bool    parseSectors(void);

// Global variables

int fileSize = 0;

uint16_t count_vertexes = 0;
uint16_t count_linedefs = 0;
uint16_t count_sidedefs = 0;
uint16_t count_sectors  = 0;

FILE        *file       = 0x0;
vertex_t    *vertexes   = 0x0;
linedef_t   *linedefs   = 0x0;
sidedef_t   *sidedefs   = 0x0;
sector_t    *sectors    = 0x0;

// \\ Header

int getFileSize() {
    int size = 0;
    int pos = ftell(file);

    fseek(file, 0, SEEK_END);
    size = ftell(file);

    fseek(file, pos, SEEK_SET);

    return size;
}

int getMinSectorFloor(int linedefidx) {
    linedef_t linedef = linedefs[linedefidx];
    int min = sectors[sidedefs[linedef.s1].sector].height_floor;

    if(linedefs[linedefidx].s2 == SIDE_NULL) return min;

    int min2 = sectors[sidedefs[linedef.s2].sector].height_floor;

    printf("getMinSectorFloor{%i}(%i, %i)", linedefidx, min, min2);

    if(min2 < min) min = min2;

    return min;
}

int getMaxSectorCeil(int linedefidx) {
    linedef_t linedef = linedefs[linedefidx];
    int max = sectors[sidedefs[linedef.s1].sector].height_ceil;

    if(linedef.s2 == SIDE_NULL) return max;

    int max2 = sectors[sidedefs[linedef.s2].sector].height_ceil;

    printf("getMaxSectorCeil{%i}(%i, %i)", linedefidx, max, max2);

    if(max2 > max) max = max2;

    return max;
}

int16_t getLinedefAngle(int linedefidx) {
    linedef_t linedef = linedefs[linedefidx]; // TODO: Проще передать указатель
    vertex_t v1 = vertexes[linedef.v1];
    vertex_t v2 = vertexes[linedef.v2];

    int16_t angle = atan2(v2.y - v1.y, v2.x - v2.x);
    
    return angle; // ±1: |, 0: —
}


int main() {
    printf(" # == Boom 2 DooM3 maps converter == #\n\t\tby PROPHESSOR (2018)\n\n\n");

    printf("Sizes of structures:\n\
            \tvertex_t:\t%lu\tbytes;\
            \tlinedef_t:\t%lu\tbytes;\
            \tsidedef_t:\t%lu\tbytes;\
            \tsector_t:\t%lu\tbytes;\
            \n\n", sizeof(vertex_t), sizeof(linedef_t), sizeof(sidedef_t), sizeof(sector_t));

    if(!parseVertexes()) return 1;
    printf(" ===> OK! <===\n\n");
    if(!parseLinedefs()) return 1;
    printf(" ===> OK! <===\n\n");
    if(!parseSidedefs()) return 1;
    printf(" ===> OK! <===\n\n");
    if(!parseSectors()) return 1;
    printf(" ===> OK! <===\n\n");

    printf("Generating DooM 3 map file...\n");
    file = fopen("output/map.map", "w");
        fprintf(file, "Version 2\n// entity 0\n");
        fprintf(file, "{\n\"classname\" \"worldspawn\"\n");

        for(uint16_t i = 0; i < count_linedefs; i++) {
            fprintf(file, "{\n brushDef3\n {\n");
            // TODO:
            for(short j = 0; j < 6; j++) {
                int16_t angle = getLinedefAngle(i);
                int floor = getMinSectorFloor(i);
                int ceil = getMaxSectorCeil(i);
                bool isHorizontal = !angle;
                bool isVertical   = !(abs(angle) - 1);
                vertex_t v1 = vertexes[linedefs[i].v1];
                vertex_t v2 = vertexes[linedefs[i].v2];


                switch(j) { // View from top
                    case 0: // Front
                        fprintf(file, "  ( 0 0 -1 %i )", floor);
                        break;
                    case 1: // Right
                        if(isHorizontal)    fprintf(file, "  ( 1 0 0 %i )", v1.x);//(v1.x + v2.x) / 2);
                        else if(isVertical) fprintf(file, "  ( 1 0 0 %i )", WALL_WEIGHT);
                        else printf("Custom angles not implemented!");
                        break;
                    case 2: // Back
                        fprintf(file, "  ( 0 0 1 %i )", ceil);
                        break;
                    case 3: // Left
                        if(isHorizontal)    fprintf(file, "  ( -1 0 0 %i )", v1.x);//(v1.x + v2.x) / 2);
                        else if(isVertical) fprintf(file, "  ( -1 0 0 %i )", WALL_WEIGHT);
                        else printf("Custom angles not implemented!");
                        break;
                    case 4: // Top
                        if(isHorizontal) {
                            fprintf(file, "  ( 0 -1 0 %i )", WALL_WEIGHT);
                        } else if(isVertical) {
                            fprintf(file, "  ( 0 -1 0 %i )", v1.x);//(v1.x + v2.x) / 2);
                        } else printf("Custom angles not implemented!");
                        break;
                    case 5: // Bottom
                        if(isHorizontal) {
                            fprintf(file, "  ( 0 1 0 %i )", WALL_WEIGHT);
                        } else if(isVertical) {
                            fprintf(file, "  ( 0 1 0 %i )", v1.x);//(v1.x + v2.x) / 2);
                        } else printf("Custom angles not implemented!");
                        break;
                    default:
                        return 2;
                }
                fprintf(file, " ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) \"textures/base_wall/lfwall27d\" 0 0 0\n");
                fprintf(file, "// min: %i, max: %i, angle: %hi, vertical: %hi\n", getMinSectorFloor(i), getMaxSectorCeil(i), getLinedefAngle(i), isVertical);
            }
            fprintf(file, " }\n}\n");
        }
        fprintf(file, "}\n\n{\n\"classname\" \"info_player_start\"\n\"name\" \"info_player_start_1\"\n\"origin\" \"0 0 0\"\n\"angle\" \"180\"\n}");
        fprintf(file, "\n{\n\"classname\" \"light\"\n\"name\" \"light1\"\n\"origin\" \"0 0 0\"\n\"_color\" \"0.78 0.78 0.84\"\n\"light_radius\" \"255 255 255\"\n}");
    fclose(file);

    return 0;
}

bool parseVertexes() {
    printf("Parse vertexes:\n");
    file = fopen("input/VERTEXES.lmp", "r");
    fileSize = getFileSize();

    count_vertexes = fileSize / sizeof(vertex_t) / 2;
    printf("\tFound: %i vertexes;\n", count_vertexes);

    vertexes = (vertex_t *)malloc(sizeof(vertex_t) * count_vertexes);

    for(uint16_t i = 0, idx = 0; i < fileSize; i += sizeof(vertex_t), idx++) {
        fread(&vertexes[idx], sizeof(vertexes[idx]), 1, file);
        printf("\t\t{\n\t\t\tx:\t%hi;\n\t\t\ty:\t%hi;\n\t\t}\n", vertexes[idx].x, vertexes[idx].y);
    }

    printf("\tVertexes: %lu bytes;\n", sizeof(*vertexes) * count_vertexes);

    fclose(file);

    return true;
}

bool parseLinedefs() {
    printf("Parse linedefs:\n");
    file = fopen("input/LINEDEFS.lmp", "r");
    fileSize = getFileSize();

    count_linedefs = fileSize / sizeof(linedef_t);
    printf("\tFound: %i linedefs;\n", count_linedefs);

    linedefs = (linedef_t *)malloc(sizeof(linedef_t) * count_linedefs);

    for(uint16_t i = 0, idx = 0; i < fileSize; i += sizeof(linedef_t), idx++) {
        fread(&linedefs[idx], sizeof(linedefs[idx]), 1, file);
        printf("\t\t{\n\t\t\tv1:\t%hi;\n\t\t\tv2:\t%hi;\n\t\t\tflags:\t%hi;\n\t\t\ttype:\t%hi;\n\t\t\ttag:\t%hi;\n\t\t\ts1:\t%hi\n\t\t\ts2:\t%hi;\n\t\t}\n",
                linedefs[idx].v1,
                linedefs[idx].v2,
                linedefs[idx].flags,
                linedefs[idx].type,
                linedefs[idx].tag,
                linedefs[idx].s1,
                linedefs[idx].s2
        );
    }

    printf("\tLinedefs: %lu bytes;\n", sizeof(*linedefs) * count_linedefs);

    fclose(file);

    return true;
}

bool parseSidedefs() {
    printf("Parse sidedefs:\n");
    file = fopen("input/SIDEDEFS.lmp", "r");
    fileSize = getFileSize();

    count_sidedefs = fileSize / sizeof(sidedef_t);
    printf("\tFound: %i sidedefs;\n", count_sidedefs);

    sidedefs = (sidedef_t *)malloc(sizeof(sidedef_t) * count_sidedefs);

    for(uint16_t i = 0, idx = 0; i < fileSize; i += sizeof(sidedef_t), idx++) {
        fread(&sidedefs[idx], sizeof(sidedefs[idx]), 1, file);
        printf("\t\t{\n\t\t\toffset_x:\t%hi;\n\t\t\toffset_y:\t%hi;\n\t\t\ttexture_upper:\t%s;\n\t\t\ttexture_middle:\t%s;\n\t\t\ttexture_lower:\t%s;\n\t\t\tsector:\t%hi;\n\t\t}\n",
                sidedefs[idx].offset_x,
                sidedefs[idx].offset_y,
                sidedefs[idx].texture_upper,
                sidedefs[idx].texture_middle,
                sidedefs[idx].texture_lower,
                sidedefs[idx].sector
        );
    }

    printf("\tSidedefs: %lu bytes\n", sizeof(*sidedefs) * count_sidedefs);

    fclose(file);

    return true;
}

bool parseSectors() {
    printf("Parse sectors:\n");
    file = fopen("input/SECTORS.lmp", "r");
    fileSize = getFileSize();

    count_sectors = fileSize / sizeof(sector_t);
    printf("\tFound: %i sectors;\n", count_sectors);

    sectors = (sector_t *)malloc(sizeof(sector_t) * count_sectors);

    for(uint16_t i = 0, idx = 0; i < fileSize; i += sizeof(sector_t), idx++) {
        fread(&sectors[idx], sizeof(sectors[idx]), 1, file);
        printf("\t\t{\n\t\t\theight_floor:\t%hi;\n\t\t\theight_ceil:\t%hi;\n\t\t\ttexture_floor:\t%s;\n\t\t\ttexture_ceil:\t%s;\n\t\t\ttag:\t%hu;\n\t\t}\n",
                sectors[idx].height_floor,
                sectors[idx].height_ceil,
                sectors[idx].texture_floor,
                sectors[idx].texture_ceil,
                sectors[idx].tag
        );
    }

    printf("Sectors: %lu bytes;\n", sizeof(*sectors) * count_sectors);

    fclose(file);

    return true;
}
