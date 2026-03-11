#pragma once

#include <QString>
#include <QImage>
#include <QFile>
#include <QDataStream>

class BlpReader
{
public:
    BlpReader();
    QImage read(const QString& path);
    // Add write support
    bool write(const QImage& img, const QString& path);

private:
    struct BlpHeader {
        char magic[4];          // "BLP1"
        quint32 type;           // 1 = Paletted, 0 = JPEG
        quint32 compression;    // 0, 1, or 8 (Alpha Bits / Flags) - This is at offset 0x08
        quint32 width;          // Width at 0x0C (12)
        quint32 height;         // Height at 0x10 (16)
        quint32 pictureType;    // 3=Index, 4=Index+A1, 5=Index+A8 at 0x14 (20)
        quint32 subType;        // 1=Standard? at 0x18 (24)
        quint32 mipmapOffsets[16];
        quint32 mipmapSizes[16];
    };

    QImage readPaletted(QDataStream& stream, const BlpHeader& header);
    QImage readJpegInternal(QDataStream& stream, const BlpHeader& header);
};
