#define FACES_COUNT 12
#define FACE_LEN 5

// 1 bit is whether the path should be inversed (if so add 128), the rest is the edge number

int8_t faces[FACES_COUNT][FACE_LEN+1]{ // normal 12 faces
    {  1,   2,   3,   4, -21, 0}, 
    { 11,  12,  13,  14,  -1, 0},
    { 21,  22,  23,  24, -11, 0},
    {  5,   6,   7,  22,   4, 0},
    { 15,  16,  17,   2,  14, 0},
    { 25,  26,  27,  12,  24, 0},
    { 26,  28,  30,  19, -20, 0},
    {  6,   8,  10,  29, -30, 0},
    { 16,  18,  20,   9, -10, 0},
    { 13, -17,  18,  19, -25, 0},
    { 23, -27,  28,  29,  -5, 0},
    {  3,  -7,   8,   9, -15, 0}};

#define FACE_SPARKS_COUNT 12
#define FACE_SPARK_LEN 5

int8_t fsparks[FACE_SPARKS_COUNT][FACE_SPARK_LEN+1]{ // beams away from faces
    { -7,  15, -14,  11,  22, 0},
    {-17,  25, -24,  21,   2, 0},
    {-27,   5,  -4,   1,  12, 0},
    { -1, -13,  18,  -9,   3, 0},
    {-11, -23,  28, -19,  13, 0},
    {-21,  -3,   8, -29,  23, 0},
    {-10, -18, -25,  27,  29, 0},
    {-20, -28,  -5,   7,   9, 0},
    {-30,  -8, -15,  17,  19, 0},
    { 14, -12,  26,  20, -16, 0},
    { 24, -22,   6,  30, -26, 0},
    {  4,  -2,  16,  10,  -6, 0}};

#define CENTER_RINGS_COUNT 12
#define CENTER_RING_LEN 10

int8_t rings[FACE_SPARKS_COUNT][CENTER_RING_LEN+1]{ // center rings
    { 12,  13, -17, -16,  -9,  -8,  -6,  -5,  23,  24, 0},
    {-26, -19, -18, -16, -15,   3,   4,  22,  23, -27, 0},
    {-25,  13,  14,   2,   3,  -7,  -6, -29, -28, -26, 0},
    { 27,  24, -11,   1,   2,  15,  -9,  10, -30, -28, 0},
    {-19, -20, -10,  -8,   7,   4, -21,  11,  12,  25, 0},
    {-29,  30, -20, -18,  17,  14,  -1,  21,  22,   5, 0},
    {-12, -13,  17,  16,   9,   8,   6,   5, -23, -24, 0},
    { 26,  19,  18,  16,  15,  -3,  -4, -22, -23,  27, 0},
    { 25, -13, -14,  -2,  -3,   7,   6,  29,  28,  26, 0},
    {-27, -24,  11,  -1,  -2, -15,   9, -10,  30,  28, 0},
    { 19,  20,  10,   8,  -7,  -4,  21, -11, -12, -25, 0},
    { 29, -30,  20,  18, -17, -14,   1, -21, -22,  -5, 0}};

#define COMBINED_GROUP_COUNT (FACES_COUNT + FACE_SPARKS_COUNT + CENTER_RINGS_COUNT)

int8_t *comboEdgeGroups[COMBINED_GROUP_COUNT];

void init_group_list() {
    int I = 0;
    for (int i = 0; i < FACES_COUNT; i++) {
        comboEdgeGroups[I] = &faces[i][0];
        I++;
    }
    for (int i = 0; i < FACE_SPARKS_COUNT; i++) {
        comboEdgeGroups[I] = &fsparks[i][0];
        I++;
    }
    for (int i = 0; i < CENTER_RINGS_COUNT; i++) {
        comboEdgeGroups[I] = &rings[i][0];
        I++;
    }
}
