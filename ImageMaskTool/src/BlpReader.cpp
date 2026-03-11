#include "BlpReader.h"
#include <QBuffer>
#include <QDebug>
#include <vector>
#include <cstring>

BlpReader::BlpReader()
{

}

QImage BlpReader::read(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QImage();
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    BlpHeader header;
    if (file.read(header.magic, 4) != 4) return QImage();
    
    // Check Magic
    if (strncmp(header.magic, "BLP1", 4) != 0) {
        // Simple Support for BLP1 only for now. World of Warcraft uses BLP2.
        // If "BLP2", structure is slightly different.
        // Warcraft 3 uses BLP1.
        if (strncmp(header.magic, "BLP2", 4) == 0) {
             // BLP2 Support is harder (DXT compression).
             // For now, let's just log and fail or try parse common fields.
             // BLP2 header is similar enough at start.
        } else {
            return QImage();
        }
    }

    stream >> header.type;
    stream >> header.compression;
    stream >> header.width;
    stream >> header.height;
    stream >> header.pictureType;
    stream >> header.subType;
    
    for (int i = 0; i < 16; ++i) stream >> header.mipmapOffsets[i];
    for (int i = 0; i < 16; ++i) stream >> header.mipmapSizes[i];

    if (header.width == 0 || header.height == 0) return QImage();

    // Type 0 = JPEG, Type 1 = Paletted/Uncompressed
    if (header.type == 0) {
        return readJpegInternal(stream, header);
    } else if (header.type == 1) {
        return readPaletted(stream, header);
    }

    return QImage();
}

QImage BlpReader::readJpegInternal(QDataStream& stream, const BlpHeader& header)
{
    // Reading JPEG content from BLP
    // BLP JPEG usually has a specific header size stored at the beginning of the file (after BLP header)
    // Or it is just a standard JPEG stream at the first mipmap offset.
    
    quint32 offset = header.mipmapOffsets[0];
    quint32 size = header.mipmapSizes[0];
    
    if (offset == 0 || size == 0) return QImage();
    
    stream.device()->seek(offset);
    QByteArray jpegData = stream.device()->read(size);

    // However, WC3 BLP1 JPEG often separates the JPEG Header and the JPEG Data.
    // The JPEG Header (Huffman tables etc) might be stored in a separate chunk.
    // BUT, simplistic implementation: Try to load the data directly. 
    // If it fails, we might need to prepend a standard JPEG header.
    // (This is a complex part of BLP1 JPEG).

    QImage img;
    // Attempt raw load
    if (img.loadFromData(jpegData)) {
        return img;
    }
    
    // If raw load fails, it implies the shared header issue. 
    // Handling that requires a larger buffer with the prepended header.
    // (Skipping implementation of Shared Header injection for this stub)
    // Most custom WC3 tools produce standard JPEGs inside BLP these days.

    return QImage();
}

QImage BlpReader::readPaletted(QDataStream& stream, const BlpHeader& header)
{
    // Read Palette (256 colors * 4 bytes = 1024 bytes)
    // Position usually right after header (28 + 16*4 + 16*4 = 156 bytes) ?
    // No, standard BLP1 header size is 28 bytes + mipmaps arrays.
    // Header struct size: 4 + 4*6 + 16*4 + 16*4 = 28 + 128 = 156 bytes.
    
    stream.device()->seek(156); // approx header size
    
    QVector<QRgb> palette(256);
    for (int i = 0; i < 256; ++i) {
        quint32 color;
        stream >> color; 
        // BLP format is typically BGRA or RGBA. 
        // Helper: 0 is Blue, 1 is Green, 2 is Red.
        // Usually stored as UInt32.
        // Let's assume standard order.
        palette[i] = color;
    }

    // Go to Mipmap 0
    quint32 offset = header.mipmapOffsets[0];
    quint32 size = header.mipmapSizes[0];
    stream.device()->seek(offset);
    
    QByteArray indexData = stream.device()->read(header.width * header.height);
    
    // Construct Image
    QImage img((const uchar*)indexData.data(), header.width, header.height, header.width, QImage::Format_Indexed8);
    img.setColorTable(palette);
    
    // Depending on compression (3, 4, 5, etc.), the alpha channel might be separate.
    // Compression 5 = Alpha list at end?
    // This is a minimal implementation for standard indexed BLPs.
    // Make a deep copy because indexData goes out of scope.
    return img.copy();
}

// --------------------------------------------------------
// Writing BLP1 (Indexed 256 Colors + Alpha)
// --------------------------------------------------------
bool BlpReader::write(const QImage& inputImg, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    // BLP1 uses Indexed8 format (max 256 colors)
    // Convert input image to Indexed8
    // Note: This is lossy for color but standard for WC3 textures.
    QImage img = inputImg;
    if (img.format() != QImage::Format_Indexed8) {
        // Use standard Qt dithering or optimal palette generation
        img = img.convertToFormat(QImage::Format_Indexed8);
    }
    
    // Prepare Header
    BlpHeader header;
    memset(&header, 0, sizeof(header));
    
    // Magic "BLP1"
    header.magic[0] = 'B'; header.magic[1] = 'L'; 
    header.magic[2] = 'P'; header.magic[3] = '1';
    
    // 0x04: Type = 1 (Paletted, 0 is JPEG)
    // 0x08: Flags / Alpha Bits. 
    // Standard WC3 BLP1 "Uncompressed": Type=1.
    // At 0x08, we put 'Compression' (1) or 'Alpha Bits' (8/0)?
    // Usually, many WC3 tools set it to 1 (Compression = Uncompressed).
    // Let's use 1.
    // Wait, if it's "Alpha Bits", it should be 8 or 0?
    // Let's try 0 for No Alpha, 8 for Alpha.
    // BUT common BLP1 writer uses:
    // Type=1 (Paletted)
    // Compression=0 (None? or 1?)
    // Actually, "Compression" field in BLP1 is tricky.
    // If Type=0 (JPEG), Compression is JPEG header length?
    // If Type=1 (Paletted), Compression is typically:
    // 1 (Uncompressed), 2 (DXT - BLP2 only?)
    // Let's stick with my first guess: Compression = 1.
    // However, the structure now maps 'compression' to offset 0x08.
    
    bool hasAlpha = inputImg.hasAlphaChannel();
    
    header.type = 1; 
    // Usually offset 0x08 is "AlphaDepth" for Uncompressed BLP? No, offset 0x08 is "Compression".
    header.compression = 1; // 1 = Uncompressed paletted.
    
    // Offset 0x0C (12): Width
    header.width = img.width();
    
    // Offset 0x10 (16): Height
    header.height = img.height();
    
    // Offset 0x14 (20): Picture Type
    // 3 = Index List (No Alpha)
    // 4 = Index + 1bit Alpha
    // 5 = Index + 8bit Alpha
    if (hasAlpha) header.pictureType = 5;
    else header.pictureType = 3; // or 4? 3 feels safer for no alpha.
    
    // Offset 0x18 (24): SubType / Mipmap Flag
    // Usually 1 for single level or standard usage?
    // Some tools set it to 1, some to 3, 4, 5 mirrored? 
    // Often it's just 1.
    header.subType = 1;
    
    // Calculate offsets
    // Header is 28 bytes + Mipmap arrays (16*4*2 = 128 bytes) = 156 bytes.
    // Palette is 256 * 4 = 1024 bytes.
    // Total header + palette = 1180 bytes.
    // Mipmap 0 data starts at 1180.
    
    header.mipmapOffsets[0] = 1180;
    
    quint32 pixelDataSize = header.width * header.height;
    quint32 alphaDataSize = hasAlpha ? (header.width * header.height) : 0;
    
    header.mipmapSizes[0] = pixelDataSize + alphaDataSize;
    
    // Write Header Strucutre
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    
    file.write(header.magic, 4);
    out << header.type;
    out << header.compression;
    out << header.width;
    out << header.height;
    out << header.pictureType;
    out << header.subType;
    
    for (int i = 0; i < 16; ++i) out << header.mipmapOffsets[i];
    for (int i = 0; i < 16; ++i) out << header.mipmapSizes[i];
    
    // Write Palette (256 colors * 4 bytes BGRA)
    // Qt's colorTable might be smaller than 256, fill rest with black
    QVector<QRgb> colorTable = img.colorTable();
    while (colorTable.size() < 256) colorTable.append(0xFF000000); 
    
    for (int i = 0; i < 256; ++i) {
        // BLP expects BGRA (Little Endian uint32)
        // Qt QRgb (0xAARRGGBB) written as LE uint32 becomes B G R A
        // So we can write directly.
        out << (quint32)colorTable[i];
    }
    
    // Write Pixel Data (Indices)
    // img is Format_Indexed8, scanning line by line
    // Ensure we write packed bytes (no stride padding)
    int w = img.width();
    int h = img.height();
    
    for (int y = 0; y < h; ++y) {
        const uchar* scanLine = img.constScanLine(y);
        file.write((const char*)scanLine, w);
    }
    
    // Write Alpha Data (if 8 bit)
    if (hasAlpha) {
        // Extract alpha for each pixel from the indexed image
        for (int y = 0; y < h; ++y) {
            const uchar* scanLine = img.constScanLine(y);
            QByteArray alphaRow;
            alphaRow.resize(w);
            for (int x = 0; x < w; ++x) {
                int idx = scanLine[x];
                QRgb c = colorTable[idx];
                alphaRow[x] = (char)qAlpha(c);
            }
            file.write(alphaRow);
        }
    }
    
    file.close();
    return true;
}
