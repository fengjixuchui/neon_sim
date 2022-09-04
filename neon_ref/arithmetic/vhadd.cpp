#include "vhadd.hpp"

namespace pxl {

//----------------------------------------------------------------------
// vhadd_type
//----------------------------------------------------------------------
int8x8_t vhadd_s8(int8x8_t N, int8x8_t M)
{
    int8x8_t D;
    for (int i=0; i<8; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

int16x4_t vhadd_s16(int16x4_t N, int16x4_t M)
{
    int16x4_t D;
    for (int i=0; i<4; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

int32x2_t vhadd_s32(int32x2_t N, int32x2_t M)
{
    int32x2_t D;
    for (int i=0; i<2; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint8x8_t vhadd_u8(uint8x8_t N, uint8x8_t M)
{
    uint8x8_t D;
    for (int i=0; i<8; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint16x4_t vhadd_u16(uint16x4_t N, uint16x4_t M)
{
    uint16x4_t D;
    for (int i=0; i<4; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint32x2_t vhadd_u32(uint32x2_t N, uint32x2_t M)
{
    uint32x2_t D;
    for (int i=0; i<2; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

//----------------------------------------------------------------------
// vhaddq_type
//----------------------------------------------------------------------
int8x16_t vhaddq_s8(int8x16_t N, int8x16_t M)
{
    int8x16_t D;
    for (int i=0; i<16; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

int16x8_t vhaddq_s16(int16x8_t N, int16x8_t M)
{
    int16x8_t D;
    for (int i=0; i<8; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

int32x4_t vhaddq_s32(int32x4_t N, int32x4_t M)
{
    int32x4_t D;
    for (int i=0; i<4; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint8x16_t vhaddq_u8(uint8x16_t N, uint8x16_t M)
{
    uint8x16_t D;
    for (int i=0; i<16; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint16x8_t vhaddq_u16(uint16x8_t N, uint16x8_t M)
{
    uint16x8_t D;
    for (int i=0; i<8; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

uint32x4_t vhaddq_u32(uint32x4_t N, uint32x4_t M)
{
    uint32x4_t D;
    for (int i=0; i<4; i++)
    {
        D[i] = (N[i] + M[i]) >> 1;
    }
    return D;
}

} // namespace pxl