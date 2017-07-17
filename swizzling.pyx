# Use Cython for Python 3.4+

# Credits:
#  -AddrLib: actual code
#  -Exzap: modifying code to apply to Wii U textures
#  -AboodXD: porting, code improvements and cleaning up

cdef int m_banks = 4
cdef int m_banksBitcount = 2
cdef int m_pipes = 2
cdef int m_pipesBitcount = 1
cdef int m_pipeInterleaveBytes = 256
cdef int m_pipeInterleaveBytesBitcount = 8
cdef int m_rowSize = 2048
cdef int m_swapSize = 256
cdef int m_splitSize = 2048

cdef int m_chipFamily = 2

cdef int MicroTilePixels = 64

cdef bytes formatHwInfo = b"\x00\x00\x00\x01\x08\x03\x00\x01\x08\x01\x00\x01\x00\x00\x00\x01" \
               b"\x00\x00\x00\x01\x10\x07\x00\x00\x10\x03\x00\x01\x10\x03\x00\x01" \
               b"\x10\x0B\x00\x01\x10\x01\x00\x01\x10\x03\x00\x01\x10\x03\x00\x01" \
               b"\x10\x03\x00\x01\x20\x03\x00\x00\x20\x07\x00\x00\x20\x03\x00\x00" \
               b"\x20\x03\x00\x01\x20\x05\x00\x00\x00\x00\x00\x00\x20\x03\x00\x00" \
               b"\x00\x00\x00\x00\x00\x00\x00\x01\x20\x03\x00\x01\x00\x00\x00\x01" \
               b"\x00\x00\x00\x01\x20\x0B\x00\x01\x20\x0B\x00\x01\x20\x0B\x00\x01" \
               b"\x40\x05\x00\x00\x40\x03\x00\x00\x40\x03\x00\x00\x40\x03\x00\x00" \
               b"\x40\x03\x00\x01\x00\x00\x00\x00\x80\x03\x00\x00\x80\x03\x00\x00" \
               b"\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x01\x10\x01\x00\x00" \
               b"\x10\x01\x00\x00\x20\x01\x00\x00\x20\x01\x00\x00\x20\x01\x00\x00" \
               b"\x00\x01\x00\x01\x00\x01\x00\x00\x00\x01\x00\x00\x60\x01\x00\x00" \
               b"\x60\x01\x00\x00\x40\x01\x00\x01\x80\x01\x00\x01\x80\x01\x00\x01" \
               b"\x40\x01\x00\x01\x80\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00" \
               b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
               b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"


cpdef int surfaceGetBitsPerPixel(int surfaceFormat):
    cdef int hwFormat = surfaceFormat & 0x3F
    cdef int bpp = formatHwInfo[hwFormat * 4 + 0]

    return bpp


cdef int computeSurfaceThickness(int tileMode):
    cdef int thickness = 1

    if tileMode in [3, 7, 11, 13, 15]:
        thickness = 4

    elif tileMode in [16, 17]:
        thickness = 8

    return thickness


cdef int computePixelIndexWithinMicroTile(int x, int y, int bpp, int tileMode):
    cdef int z = 0
    cdef int thickness
    cdef int pixelBit8
    cdef int pixelBit7
    cdef int pixelBit6
    cdef int pixelBit5
    cdef int pixelBit4
    cdef int pixelBit3
    cdef int pixelBit2
    cdef int pixelBit1
    cdef int pixelBit0
    pixelBit6 = 0
    pixelBit7 = 0
    pixelBit8 = 0
    thickness = computeSurfaceThickness(tileMode)

    if bpp == 0x08:
        pixelBit0 = x & 1
        pixelBit1 = (x & 2) >> 1
        pixelBit2 = (x & 4) >> 2
        pixelBit3 = (y & 2) >> 1
        pixelBit4 = y & 1
        pixelBit5 = (y & 4) >> 2

    elif bpp == 0x10:
        pixelBit0 = x & 1
        pixelBit1 = (x & 2) >> 1
        pixelBit2 = (x & 4) >> 2
        pixelBit3 = y & 1
        pixelBit4 = (y & 2) >> 1
        pixelBit5 = (y & 4) >> 2

    elif bpp in [0x20, 0x60]:
        pixelBit0 = x & 1
        pixelBit1 = (x & 2) >> 1
        pixelBit2 = y & 1
        pixelBit3 = (x & 4) >> 2
        pixelBit4 = (y & 2) >> 1
        pixelBit5 = (y & 4) >> 2

    elif bpp == 0x40:
        pixelBit0 = x & 1
        pixelBit1 = y & 1
        pixelBit2 = (x & 2) >> 1
        pixelBit3 = (x & 4) >> 2
        pixelBit4 = (y & 2) >> 1
        pixelBit5 = (y & 4) >> 2

    elif bpp == 0x80:
        pixelBit0 = y & 1
        pixelBit1 = x & 1
        pixelBit2 = (x & 2) >> 1
        pixelBit3 = (x & 4) >> 2
        pixelBit4 = (y & 2) >> 1
        pixelBit5 = (y & 4) >> 2

    else:
        pixelBit0 = x & 1
        pixelBit1 = (x & 2) >> 1
        pixelBit2 = y & 1
        pixelBit3 = (x & 4) >> 2
        pixelBit4 = (y & 2) >> 1
        pixelBit5 = (y & 4) >> 2

    if thickness > 1:
        pixelBit6 = z & 1
        pixelBit7 = (z & 2) >> 1

    if thickness == 8:
        pixelBit8 = (z & 4) >> 2

    return ((pixelBit8 << 8) | (pixelBit7 << 7) | (pixelBit6 << 6) |
            32 * pixelBit5 | 16 * pixelBit4 | 8 * pixelBit3 |
            4 * pixelBit2 | pixelBit0 | 2 * pixelBit1)


cdef int computePipeFromCoordWoRotation(int x, int y):
    # hardcoded to assume 2 pipes
    return ((y >> 3) ^ (x >> 3)) & 1


cdef int computeBankFromCoordWoRotation(int x, int y):
    cdef int numPipes = m_pipes
    cdef int numBanks = m_banks
    cdef int bankBit0
    cdef int bankBit0a
    cdef int bank = 0

    if numBanks == 4:
        bankBit0 = ((y // (16 * numPipes)) ^ (x >> 3)) & 1
        bank = bankBit0 | 2 * (((y // (8 * numPipes)) ^ (x >> 4)) & 1)

    elif numBanks == 8:
        bankBit0a = ((y // (32 * numPipes)) ^ (x >> 3)) & 1
        bank = (bankBit0a | 2 * (((y // (32 * numPipes)) ^ (y // (16 * numPipes) ^ (x >> 4))) & 1) |
                4 * (((y // (8 * numPipes)) ^ (x >> 5)) & 1))

    return bank


cdef int isThickMacroTiled(int tileMode):
    cdef int thickMacroTiled = 0

    if tileMode in [7, 11, 13, 15]:
        thickMacroTiled = 1

    return thickMacroTiled


cdef int isBankSwappedTileMode(int tileMode):
    cdef int bankSwapped = 0

    if tileMode in [8, 9, 10, 11, 14, 15]:
        bankSwapped = 1

    return bankSwapped


cdef int computeMacroTileAspectRatio(int tileMode):
    cdef int ratio = 1

    if tileMode in [8, 12, 14]:
        ratio = 1

    elif tileMode in [5, 9]:
        ratio = 2

    elif tileMode in [6, 10]:
        ratio = 4

    return ratio


cdef int computeSurfaceBankSwappedWidth(int tileMode, int bpp, int pitch):
    if isBankSwappedTileMode(tileMode) == 0:
        return 0

    cdef int numSamples = 1
    cdef int numBanks = m_banks
    cdef int numPipes = m_pipes
    cdef int swapSize = m_swapSize
    cdef int rowSize = m_rowSize
    cdef int splitSize = m_splitSize
    cdef int groupSize = m_pipeInterleaveBytes
    cdef int bytesPerSample = 8 * bpp
    cdef int samplesPerTile
    cdef int slicesPerTile

    if bytesPerSample != 0:
        samplesPerTile = splitSize // bytesPerSample
        slicesPerTile = max(1, numSamples // samplesPerTile)
    else:
        slicesPerTile = 1

    if isThickMacroTiled(tileMode) != 0:
        numSamples = 4

    cdef int bytesPerTileSlice = numSamples * bytesPerSample // slicesPerTile

    cdef int factor = computeMacroTileAspectRatio(tileMode)
    cdef int swapTiles = max(1, (swapSize >> 1) // bpp)

    cdef int swapWidth = swapTiles * 8 * numBanks
    cdef int heightBytes = numSamples * factor * numPipes * bpp // slicesPerTile
    cdef int swapMax = numPipes * numBanks * rowSize // heightBytes
    cdef int swapMin = groupSize * 8 * numBanks // bytesPerTileSlice

    cdef int bankSwapWidth = min(swapMax, max(swapMin, swapWidth))

    while not bankSwapWidth < (2 * pitch):
        bankSwapWidth >>= 1

    return bankSwapWidth


cpdef int AddrLib_computeSurfaceAddrFromCoordLinear(int x, int y, int bpp, int pitch):
    cdef int rowOffset = y * pitch
    cdef int pixOffset = x

    cdef int addr = (rowOffset + pixOffset) * bpp
    addr //= 8

    return addr


cpdef int AddrLib_computeSurfaceAddrFromCoordMicroTiled(int x, int y, int bpp, int pitch, int tileMode):
    cdef int microTileThickness = 1

    if tileMode == 3:
        microTileThickness = 4

    cdef int microTileBytes = (MicroTilePixels * microTileThickness * bpp + 7) // 8
    cdef int microTilesPerRow = pitch >> 3
    cdef int microTileIndexX = x >> 3
    cdef int microTileIndexY = y >> 3

    cdef int microTileOffset = microTileBytes * (microTileIndexX + microTileIndexY * microTilesPerRow)

    cdef int pixelIndex = computePixelIndexWithinMicroTile(x, y, bpp, tileMode)

    cdef int pixelOffset = bpp * pixelIndex

    pixelOffset >>= 3

    return pixelOffset + microTileOffset


cpdef int AddrLib_computeSurfaceAddrFromCoordMacroTiled(int x, int y, int bpp, int pitch, int height, int tileMode, int pipeSwizzle, int bankSwizzle):
    cdef int sampleSlice
    cdef int numSamples
    cdef int samplesPerSlice
    cdef int numSampleSplits
    cdef list bankSwapOrder
    cdef int bankSwapWidth
    cdef int swapIndex

    cdef int numPipes = m_pipes
    cdef int numBanks = m_banks
    cdef int numGroupBits = m_pipeInterleaveBytesBitcount
    cdef int numPipeBits = m_pipesBitcount
    cdef int numBankBits = m_banksBitcount

    cdef int microTileThickness = computeSurfaceThickness(tileMode)

    cdef int microTileBits = bpp * (microTileThickness * MicroTilePixels)
    cdef int microTileBytes = (microTileBits + 7) // 8

    cdef int pixelIndex = computePixelIndexWithinMicroTile(x, y, bpp, tileMode)

    cdef int pixelOffset = bpp * pixelIndex

    cdef int elemOffset = pixelOffset

    cdef int bytesPerSample = microTileBytes
    if microTileBytes <= m_splitSize:
        numSamples = 1
        sampleSlice = 0
    else:
        samplesPerSlice = m_splitSize // bytesPerSample
        numSampleSplits = max(1, 1 // samplesPerSlice)
        numSamples = samplesPerSlice
        sampleSlice = elemOffset // (microTileBits // numSampleSplits)
        elemOffset %= microTileBits // numSampleSplits
    elemOffset += 7
    elemOffset //= 8

    cdef int pipe = computePipeFromCoordWoRotation(x, y)
    cdef int bank = computeBankFromCoordWoRotation(x, y)

    cdef int bankPipe = pipe + numPipes * bank

    cdef int swizzle_ = pipeSwizzle + numPipes * bankSwizzle

    bankPipe ^= numPipes * sampleSlice * ((numBanks >> 1) + 1) ^ swizzle_
    bankPipe %= numPipes * numBanks
    pipe = bankPipe % numPipes
    bank = bankPipe // numPipes

    cdef int sliceBytes = (height * pitch * microTileThickness * bpp * numSamples + 7) // 8
    cdef int sliceOffset = sliceBytes * (sampleSlice // microTileThickness)

    cdef int macroTilePitch = 8 * m_banks
    cdef int macroTileHeight = 8 * m_pipes

    if tileMode in [5, 9]:  # GX2_TILE_MODE_2D_TILED_THIN4 and GX2_TILE_MODE_2B_TILED_THIN2
        macroTilePitch >>= 1
        macroTileHeight *= 2

    elif tileMode in [6, 10]:  # GX2_TILE_MODE_2D_TILED_THIN4 and GX2_TILE_MODE_2B_TILED_THIN4
        macroTilePitch >>= 2
        macroTileHeight *= 4

    cdef int macroTilesPerRow = pitch // macroTilePitch
    cdef int macroTileBytes = (numSamples * microTileThickness * bpp * macroTileHeight * macroTilePitch + 7) // 8
    cdef int macroTileIndexX = x // macroTilePitch
    cdef int macroTileIndexY = y // macroTileHeight
    cdef int macroTileOffset = (macroTileIndexX + macroTilesPerRow * macroTileIndexY) * macroTileBytes

    if tileMode in [8, 9, 10, 11, 14, 15]:
        bankSwapOrder = [0, 1, 3, 2, 6, 7, 5, 4, 0, 0]
        bankSwapWidth = computeSurfaceBankSwappedWidth(tileMode, bpp, pitch)
        swapIndex = macroTilePitch * macroTileIndexX // bankSwapWidth
        bank ^= bankSwapOrder[swapIndex & (m_banks - 1)]

    cdef int groupMask = ((1 << numGroupBits) - 1)

    cdef int numSwizzleBits = (numBankBits + numPipeBits)

    cdef int totalOffset = (elemOffset + ((macroTileOffset + sliceOffset) >> numSwizzleBits))

    cdef int offsetHigh = (totalOffset & ~groupMask) << numSwizzleBits
    cdef int offsetLow = groupMask & totalOffset

    cdef int pipeBits = pipe << numGroupBits
    cdef int bankBits = bank << (numPipeBits + numGroupBits)

    return bankBits | pipeBits | offsetLow | offsetHigh
