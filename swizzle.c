/* Start of swizzling section */

/* Credits:
  -AddrLib: actual code
  -Exzap: modifying code to apply to Wii U textures
  -AboodXD: porting, code improvements and cleaning up
*/

static uint32_t m_banks = 4;
static uint32_t m_banksBitcount = 2;
static uint32_t m_pipes = 2;
static uint32_t m_pipesBitcount = 1;
static uint32_t m_pipeInterleaveBytes = 256;
static uint32_t m_pipeInterleaveBytesBitcount = 8;
static uint32_t m_rowSize = 2048;
static uint32_t m_swapSize = 256;
static uint32_t m_splitSize = 2048;

static uint32_t m_chipFamily = 2;

static uint32_t MicroTilePixels = 8 * 8;

uint8_t formatHwInfo[0x40*4] =
{
	// todo: Convert to struct
	// each entry is 4 bytes
	0x00,0x00,0x00,0x01,0x08,0x03,0x00,0x01,0x08,0x01,0x00,0x01,0x00,0x00,0x00,0x01,
	0x00,0x00,0x00,0x01,0x10,0x07,0x00,0x00,0x10,0x03,0x00,0x01,0x10,0x03,0x00,0x01,
	0x10,0x0B,0x00,0x01,0x10,0x01,0x00,0x01,0x10,0x03,0x00,0x01,0x10,0x03,0x00,0x01,
	0x10,0x03,0x00,0x01,0x20,0x03,0x00,0x00,0x20,0x07,0x00,0x00,0x20,0x03,0x00,0x00,
	0x20,0x03,0x00,0x01,0x20,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x03,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x20,0x03,0x00,0x01,0x00,0x00,0x00,0x01,
	0x00,0x00,0x00,0x01,0x20,0x0B,0x00,0x01,0x20,0x0B,0x00,0x01,0x20,0x0B,0x00,0x01,
	0x40,0x05,0x00,0x00,0x40,0x03,0x00,0x00,0x40,0x03,0x00,0x00,0x40,0x03,0x00,0x00,
	0x40,0x03,0x00,0x01,0x00,0x00,0x00,0x00,0x80,0x03,0x00,0x00,0x80,0x03,0x00,0x00,
	0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x10,0x01,0x00,0x00,
	0x10,0x01,0x00,0x00,0x20,0x01,0x00,0x00,0x20,0x01,0x00,0x00,0x20,0x01,0x00,0x00,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x60,0x01,0x00,0x00,
	0x60,0x01,0x00,0x00,0x40,0x01,0x00,0x01,0x80,0x01,0x00,0x01,0x80,0x01,0x00,0x01,
	0x40,0x01,0x00,0x01,0x80,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

uint32_t surfaceGetBitsPerPixel(uint32_t surfaceFormat)
{
	uint32_t hwFormat = surfaceFormat&0x3F;
	uint32_t bpp = formatHwInfo[hwFormat*4+0];
	return bpp;
}

uint32_t computeSurfaceThickness(uint32_t tileMode)
{
    uint32_t thickness = 1;

    if (tileMode == 3 || tileMode == 7 || tileMode == 11 || tileMode == 13 || tileMode == 15)
        thickness = 4;

    else if (tileMode == 16 || tileMode == 17)
        thickness = 8;

    return thickness;
}

uint32_t computePixelIndexWithinMicroTile(uint32_t x, uint32_t y, uint32_t bpp, uint32_t tileMode)
{
    uint32_t z = 0;
    uint32_t thickness;
	uint32_t pixelBit8;
	uint32_t pixelBit7;
	uint32_t pixelBit6;
	uint32_t pixelBit5;
	uint32_t pixelBit4;
	uint32_t pixelBit3;
	uint32_t pixelBit2;
	uint32_t pixelBit1;
	uint32_t pixelBit0;
    pixelBit6 = 0;
    pixelBit7 = 0;
    pixelBit8 = 0;
    thickness = computeSurfaceThickness(tileMode);

    if (bpp == 0x08) {
        pixelBit0 = x & 1;
        pixelBit1 = (x & 2) >> 1;
        pixelBit2 = (x & 4) >> 2;
        pixelBit3 = (y & 2) >> 1;
        pixelBit4 = y & 1;
        pixelBit5 = (y & 4) >> 2;
    }

    else if (bpp == 0x10) {
        pixelBit0 = x & 1;
        pixelBit1 = (x & 2) >> 1;
        pixelBit2 = (x & 4) >> 2;
        pixelBit3 = y & 1;
        pixelBit4 = (y & 2) >> 1;
        pixelBit5 = (y & 4) >> 2;
    }

    else if (bpp == 0x20 || bpp == 0x60) {
        pixelBit0 = x & 1;
        pixelBit1 = (x & 2) >> 1;
        pixelBit2 = y & 1;
        pixelBit3 = (x & 4) >> 2;
        pixelBit4 = (y & 2) >> 1;
        pixelBit5 = (y & 4) >> 2;
    }

    else if (bpp == 0x40) {
        pixelBit0 = x & 1;
        pixelBit1 = y & 1;
        pixelBit2 = (x & 2) >> 1;
        pixelBit3 = (x & 4) >> 2;
        pixelBit4 = (y & 2) >> 1;
        pixelBit5 = (y & 4) >> 2;
    }

    else if (bpp == 0x80) {
        pixelBit0 = y & 1;
        pixelBit1 = x & 1;
        pixelBit2 = (x & 2) >> 1;
        pixelBit3 = (x & 4) >> 2;
        pixelBit4 = (y & 2) >> 1;
        pixelBit5 = (y & 4) >> 2;
    }

    else {
        pixelBit0 = x & 1;
        pixelBit1 = (x & 2) >> 1;
        pixelBit2 = y & 1;
        pixelBit3 = (x & 4) >> 2;
        pixelBit4 = (y & 2) >> 1;
        pixelBit5 = (y & 4) >> 2;
    }

    if (thickness > 1) {
        pixelBit6 = z & 1;
        pixelBit7 = (z & 2) >> 1;
    }

    if (thickness == 8)
        pixelBit8 = (z & 4) >> 2;

    return ((pixelBit8 << 8) | (pixelBit7 << 7) | (pixelBit6 << 6) |
            32 * pixelBit5 | 16 * pixelBit4 | 8 * pixelBit3 |
            4 * pixelBit2 | pixelBit0 | 2 * pixelBit1);
}

uint32_t computePipeFromCoordWoRotation(uint32_t x, uint32_t y) {
    // hardcoded to assume 2 pipes
    uint32_t pipe = ((y >> 3) ^ (x >> 3)) & 1;
    return pipe;
}

uint32_t computeBankFromCoordWoRotation(uint32_t x, uint32_t y) {
    uint32_t numPipes = m_pipes;
    uint32_t numBanks = m_banks;
    uint32_t bankBit0;
    uint32_t bankBit0a;
    uint32_t bank = 0;

    if (numBanks == 4) {
        bankBit0 = ((y / (16 * numPipes)) ^ (x >> 3)) & 1;
        bank = bankBit0 | 2 * (((y / (8 * numPipes)) ^ (x >> 4)) & 1);
    }

    else if (numBanks == 8) {
        bankBit0a = ((y / (32 * numPipes)) ^ (x >> 3)) & 1;
        bank = bankBit0a | 2 * (((y / (32 * numPipes)) ^ (y / (16 * numPipes) ^ (x >> 4))) & 1) | 4 * (((y / (8 * numPipes)) ^ (x >> 5)) & 1);
    }

    return bank;
}

uint32_t computeSurfaceRotationFromTileMode(uint32_t tileMode) {
    uint32_t pipes = m_pipes;
    uint32_t result = 0;

    if (tileMode == 4 || tileMode == 5 || tileMode == 6 || tileMode == 7 || tileMode == 8 || tileMode == 9 || tileMode == 10 || tileMode == 11)
        result = pipes * ((m_banks >> 1) - 1);

    else if (tileMode == 12 || tileMode == 13 || tileMode == 14 || tileMode == 15) {
        if (pipes >= 4)
            result = (pipes >> 1) - 1;

        else
            result = 1;
    }

    return result;
}

uint32_t isThickMacroTiled(uint32_t tileMode) {
    uint32_t thickMacroTiled = 0;

    if (tileMode == 7 || tileMode == 11 || tileMode == 13 || tileMode == 15)
        thickMacroTiled = 1;

    return thickMacroTiled;
}

uint32_t isBankSwappedTileMode(uint32_t tileMode) {
    uint32_t bankSwapped = 0;

    if (tileMode == 8 || tileMode == 9 || tileMode == 10 || tileMode == 11 || tileMode == 14 || tileMode == 15)
        bankSwapped = 1;

    return bankSwapped;
}

uint32_t computeMacroTileAspectRatio(uint32_t tileMode) {
    uint32_t ratio = 1;

    if (tileMode == 5 || tileMode == 9)
        ratio = 2;

    else if (tileMode == 6 || tileMode == 10)
        ratio = 4;

    return ratio;
}

uint32_t computeSurfaceBankSwappedWidth(uint32_t tileMode, uint32_t bpp, uint32_t pitch) {
    if (isBankSwappedTileMode(tileMode) == 0)
        return 0;

    uint32_t numSamples = 1;
    uint32_t numBanks = m_banks;
    uint32_t numPipes = m_pipes;
    uint32_t swapSize = m_swapSize;
    uint32_t rowSize = m_rowSize;
    uint32_t splitSize = m_splitSize;
    uint32_t groupSize = m_pipeInterleaveBytes;
    uint32_t bytesPerSample = 8 * bpp;

    uint32_t samplesPerTile = splitSize / bytesPerSample;
    uint32_t slicesPerTile = max(1, numSamples / samplesPerTile);

    if (isThickMacroTiled(tileMode) != 0)
        numSamples = 4;

    uint32_t bytesPerTileSlice = numSamples * bytesPerSample / slicesPerTile;

    uint32_t factor = computeMacroTileAspectRatio(tileMode);
    uint32_t swapTiles = max(1, (swapSize >> 1) / bpp);

    uint32_t swapWidth = swapTiles * 8 * numBanks;
    uint32_t heightBytes = numSamples * factor * numPipes * bpp / slicesPerTile;
    uint32_t swapMax = numPipes * numBanks * rowSize / heightBytes;
    uint32_t swapMin = groupSize * 8 * numBanks / bytesPerTileSlice;

    uint32_t bankSwapWidth = min(swapMax, max(swapMin, swapWidth));

    while (bankSwapWidth >= (2 * pitch))
        bankSwapWidth >>= 1;

    return bankSwapWidth;
}

uint64_t AddrLib_computeSurfaceAddrFromCoordLinear(uint32_t x, uint32_t y, uint32_t bpp, uint32_t pitch, uint32_t height) {
    uint32_t rowOffset = y * pitch;
    uint32_t pixOffset = x;

    uint32_t addr = (rowOffset + pixOffset) * bpp;
    addr /= 8;

    return addr;
}

uint64_t AddrLib_computeSurfaceAddrFromCoordMicroTiled(uint32_t x, uint32_t y, uint32_t bpp, uint32_t pitch, uint32_t height, uint32_t tileMode) {
    uint64_t microTileThickness = 1;

    if (tileMode == 3)
        microTileThickness = 4;

    uint64_t microTileBytes = (MicroTilePixels * microTileThickness * bpp + 7) / 8;
    uint64_t microTilesPerRow = pitch >> 3;
    uint64_t microTileIndexX = x >> 3;
    uint64_t microTileIndexY = y >> 3;

    uint64_t microTileOffset = microTileBytes * (microTileIndexX + microTileIndexY * microTilesPerRow);

    uint64_t pixelIndex = computePixelIndexWithinMicroTile(x, y, bpp, tileMode);

    uint64_t pixelOffset = bpp * pixelIndex;

    pixelOffset >>= 3;

    return pixelOffset + microTileOffset;
}

uint64_t AddrLib_computeSurfaceAddrFromCoordMacroTiled(uint32_t x, uint32_t y, uint32_t bpp, uint32_t pitch, uint32_t height, uint32_t tileMode, uint32_t pipeSwizzle, uint32_t bankSwizzle) {
    uint32_t numPipes = m_pipes;
    uint32_t numBanks = m_banks;
    uint32_t numGroupBits = m_pipeInterleaveBytesBitcount;
    uint32_t numPipeBits = m_pipesBitcount;
    uint32_t numBankBits = m_banksBitcount;

    uint32_t microTileThickness = computeSurfaceThickness(tileMode);

    uint64_t pixelIndex = computePixelIndexWithinMicroTile(x, y, bpp, tileMode);

    uint64_t elemOffset = (bpp * pixelIndex) >> 3;

    uint64_t pipe = computePipeFromCoordWoRotation(x, y);
    uint64_t bank = computeBankFromCoordWoRotation(x, y);

    uint64_t bankPipe = pipe + numPipes * bank;
    uint64_t rotation = computeSurfaceRotationFromTileMode(tileMode);

    bankPipe %= numPipes * numBanks;
    pipe = bankPipe % numPipes;
    bank = bankPipe / numPipes;

    uint64_t macroTilePitch = 8 * m_banks;
    uint64_t macroTileHeight = 8 * m_pipes;

    if (tileMode == 5 || tileMode == 9) { // GX2_TILE_MODE_2D_TILED_THIN4 and GX2_TILE_MODE_2B_TILED_THIN2
        macroTilePitch >>= 1;
        macroTileHeight *= 2;
    }

    else if (tileMode == 6 || tileMode == 10) { // GX2_TILE_MODE_2D_TILED_THIN4 and GX2_TILE_MODE_2B_TILED_THIN4
        macroTilePitch >>= 2;
        macroTileHeight *= 4;
    }

    uint64_t macroTilesPerRow = pitch / macroTilePitch;
    uint64_t macroTileBytes = (microTileThickness * bpp * macroTileHeight * macroTilePitch + 7) / 8;
    uint64_t macroTileIndexX = x / macroTilePitch;
    uint64_t macroTileIndexY = y / macroTileHeight;
    uint64_t macroTileOffset = macroTileBytes * (macroTileIndexX + macroTilesPerRow * macroTileIndexY);

    if (tileMode == 8 || tileMode == 9 || tileMode == 10 || tileMode == 11 || tileMode == 14 || tileMode == 15) {
        static const uint32_t bankSwapOrder[] = { 0, 1, 3, 2, 6, 7, 5, 4, 0, 0 };
        uint64_t bankSwapWidth = computeSurfaceBankSwappedWidth(tileMode, bpp, pitch);
        uint64_t swapIndex = macroTilePitch * macroTileIndexX / bankSwapWidth;
        bank ^= bankSwapOrder[swapIndex & (m_banks - 1)];
    }

    uint64_t group_mask = (1 << numGroupBits) - 1;
    uint64_t total_offset = elemOffset + (macroTileOffset >> (numBankBits + numPipeBits));

    uint64_t offset_high = (total_offset & ~(group_mask)) << (numBankBits + numPipeBits);
    uint64_t offset_low = total_offset & group_mask;
    uint64_t bank_bits = bank << (numPipeBits + numGroupBits);
    uint64_t pipe_bits = pipe << numGroupBits;

    return bank_bits | pipe_bits | offset_low | offset_high;
}
