#include "ImageProcessor.h"
#include "BlpReader.h"
#include <QPainter>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QBuffer>

#pragma pack(push, 1)
struct TgaHeader {
    quint8  idLength = 0;
    quint8  colorMapType = 0;
    quint8  imageType = 2; // Uncompressed True-Color Image
    quint16 colorMapOrigin = 0;
    quint16 colorMapLength = 0;
    quint8  colorMapDepth = 0;
    quint16 xOrigin = 0;
    quint16 yOrigin = 0;
    quint16 width = 0;
    quint16 height = 0;
    quint8  pixelDepth = 32;
    quint8  imageDescriptor = 0x20 | 8; // Top-left origin, 8 alpha bits
};
#pragma pack(pop)

bool writeTGA(const QImage& inputImg, const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // Convert to Format_ARGB32 (0xAARRGGBB in register, BGRA in memory on Little Endian)
    // TGA expects BGRA pixel data for 32-bit images.
    QImage img = inputImg.convertToFormat(QImage::Format_ARGB32);

    // Note: Alpha inversion logic has been moved to caller (processImage) to precise control.
    // Standard TGA expects 0=Transparent, 255=Opaque.

    TgaHeader header;
    header.width = (quint16)img.width();
    header.height = (quint16)img.height();
    header.pixelDepth = 32;
    header.imageDescriptor = 0x20 | 8; // Descriptor: bit 5 set = top-down, bits 0-3 = 8 alpha bits

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian); // TGA headers are Little Endian

    // Write Header
    out << header.idLength;
    out << header.colorMapType;
    out << header.imageType;
    out << header.colorMapOrigin;
    out << header.colorMapLength;
    out << header.colorMapDepth;
    out << header.xOrigin;
    out << header.yOrigin;
    out << header.width;
    out << header.height;
    out << header.pixelDepth;
    out << header.imageDescriptor;

    // Write Pixel Data
    // QImage scanlines are aligned to 4 bytes, TGA usually expects packed or aligned. 
    // Since we are writing raw bytes, we can iterate scanlines.
    // For 32-bit (4 bytes per pixel), scanlines are naturally aligned to 4 bytes anyway in QImage.
    
    const int height = img.height();
    const int width = img.width();
    
    // Check if stride matches width * 4 (packed), if so we can write bulk
    if (img.bytesPerLine() == width * 4) {
        // Fast path: Write whole block
        file.write((const char*)img.bits(), img.sizeInBytes());
    } else {
        // Safe path: Write line by line to skip padding
        for (int y = 0; y < height; ++y) {
            file.write((const char*)img.scanLine(y), width * 4);
        }
    }

    file.close();
    return true;
}


// Custom TGA Reader
QImage ImageProcessor::loadTGA(const QString& path, bool invertAlpha) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QImage();
    
    TgaHeader header;
    if (file.read((char*)&header, sizeof(TgaHeader)) != sizeof(TgaHeader)) return QImage();
    
    // Skip ID info
    if (header.idLength > 0) file.seek(sizeof(TgaHeader) + header.idLength);

    // Support Uncompressed (2) and RLE (10) - but start with Uncompressed
    if (header.imageType != 2) {
        // Simple fallback/error for now
        qWarning() << "Unsupported TGA type:" << header.imageType;
        return QImage();
    }
    
    QImage::Format format = QImage::Format_Invalid;
    int bytesPerPixel = 0;
    
    if (header.pixelDepth == 32) {
        // Check descriptor for alpha bits
        int alphaBits = header.imageDescriptor & 0x0F;
        if (alphaBits == 0) {
            // 32-bit but 0 alpha bits -> treat alpha as dummy (RGB32)
            // This prevents "White Mask" issue where alpha is 255 but ignored by creator.
            // If user explicitly wants alpha inversion, maybe we adhere? No, inverted dummy is still dummy.
            format = QImage::Format_RGB32;
        } else {
            format = QImage::Format_ARGB32;
        }
        bytesPerPixel = 4;
    } else if (header.pixelDepth == 24) {
        format = QImage::Format_RGB888;
        bytesPerPixel = 3;
    } else {
        return QImage();
    }

    QImage img(header.width, header.height, format);
    if (img.isNull()) return QImage();

    // Read pixel data
    const int h = header.height;
    const int w = header.width;
    const int bytesPerLine = w * bytesPerPixel;
    
    // Image Descriptor bit 5: 0=Bottom-Left, 1=Top-Left
    bool isTopLeft = (header.imageDescriptor & 0x20);

    for (int i = 0; i < h; ++i) {
        int y = isTopLeft ? i : (h - 1 - i);
        uchar* scanline = img.scanLine(y);
        if (file.read((char*)scanline, bytesPerLine) != bytesPerLine) {
            return QImage(); // Read error
        }
    }

    // [修正] 读取 TGA 后反转 Alpha 通道 (如果启用)
    if (invertAlpha && format == QImage::Format_ARGB32) {
        for (int y = 0; y < h; ++y) {
            quint32* line = reinterpret_cast<quint32*>(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                line[x] ^= 0xFF000000;
            }
        }
    }

    return img;
}

ImageProcessor::ImageProcessor()
{
}

bool ImageProcessor::loadMask(const QString& maskPath, bool invertAlpha)
{
    m_maskImage = loadImageHelper(maskPath, invertAlpha);
    return !m_maskImage.isNull();
}

bool ImageProcessor::loadBase(const QString& basePath, bool invertAlpha)
{
    m_baseImage = loadImageHelper(basePath, invertAlpha);
    return !m_baseImage.isNull();
}

QImage ImageProcessor::loadImageHelper(const QString& path, bool invertAlpha)
{
    QFileInfo fi(path);
    QString ext = fi.suffix().toLower();
    
    if (ext == "blp") {
        BlpReader blp;
        // Should we invert BLP alpha if requested? Typically BLP is fixed.
        return blp.read(path);
    } else if (ext == "tga") {
        // Use flag if we pass it, otherwise false (Standard)
        // Wait, loadImageHelper doesn't have the flag yet.
        // Let's modify ImageProcessor to store the flag or pass it.
        // For simplicity, let's assume reading is standard unless we upgrade loadImageHelper.
        // But loadTGA needs 2 args now? No, default is false.
        QImage img = loadTGA(path, invertAlpha); 
        if (!img.isNull()) return img;
        return QImage();
    } else {
        QImage img;
        if (img.load(path)) {
            return img;
        }
    }
    return QImage();
}

QString ImageProcessor::processImage(const QString& inputPath, const QString& outputPath, int blendMode, bool invertTgaAlpha)
{
    // Try to load
    // Use the inversion flag for input TGA as well if desired.
    // If user checks "Invert TGA", we assume they are working in "Warcraft Mode" 
    // where TGA Alphas are inverted.
    QImage inputImg = loadImageHelper(inputPath, invertTgaAlpha);
    
    if (inputImg.isNull()) {
        return "无法加载图片 (可能缺少格式插件): " + QFileInfo(inputPath).fileName();
    }

    if (m_baseImage.isNull()) {
        return "底图未加载。";
    }

    if (m_maskImage.isNull()) {
        return "遮罩未加载。";
    }

    // Convert to ARGB32_Premultiplied for best rendering performance
    if (inputImg.format() != QImage::Format_ARGB32_Premultiplied) {
        inputImg = inputImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QImage baseImg = m_baseImage.scaled(inputImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (baseImg.format() != QImage::Format_ARGB32_Premultiplied) {
        baseImg = baseImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    // Scale mask to fit input image
    QImage scaledMask = m_maskImage.scaled(inputImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // If mask has no alpha (e.g. JPG, BMP without alpha), generate alpha from RGB brightness.
    // This allows using black-and-white images as direct transparency masks.
    if (!scaledMask.hasAlphaChannel()) {
        QImage alphaChannelMask = scaledMask.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < alphaChannelMask.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(alphaChannelMask.scanLine(y));
            for (int x = 0; x < alphaChannelMask.width(); ++x) {
                // Use luminance (or simple average) as alpha
                int alpha = qGray(line[x]);
                
                // IMPORTANT: For alpha blending, usually we want the RGB to be White (or the source color).
                // If the mask was Black (0,0,0) and we set Alpha=0, it becomes Transparent Black.
                // If the mask was White (255,255,255) and we set Alpha=255, it becomes Opaque White.
                // This preserves the "Color" of the mask too.
                line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), alpha);
            }
        }
        scaledMask = alphaChannelMask;
    }

    QImage resultImg = baseImg;
    QPainter painter(&resultImg);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    // Layer order: Base (bottom) -> Original -> Mask (top).
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, baseImg);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, inputImg);

    if (blendMode == 0) { // Alpha Mask Mode (DestinationIn)
        // Top mask controls final alpha of the current stack.
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.drawImage(0, 0, scaledMask);
        
    } else {
         // Draw mask as the top layer with selected blend mode.
         switch (blendMode) {
            case 1: painter.setCompositionMode(QPainter::CompositionMode_SourceOver); break; // Normal
            case 2: painter.setCompositionMode(QPainter::CompositionMode_Multiply); break;   // Multiply
            case 3: painter.setCompositionMode(QPainter::CompositionMode_Screen); break;     // Screen
            case 4: painter.setCompositionMode(QPainter::CompositionMode_Overlay); break;    // Overlay
            case 5: painter.setCompositionMode(QPainter::CompositionMode_Darken); break;     // Darken
            case 6: painter.setCompositionMode(QPainter::CompositionMode_Lighten); break;    // Lighten
            case 7: painter.setCompositionMode(QPainter::CompositionMode_Plus); break;       // Add
            default: painter.setCompositionMode(QPainter::CompositionMode_SourceOver); break;
         }
         
         if (scaledMask.format() != QImage::Format_ARGB32_Premultiplied) {
             scaledMask = scaledMask.convertToFormat(QImage::Format_ARGB32_Premultiplied);
         }
         painter.drawImage(0, 0, scaledMask);
    }
    
    painter.end();

    // Custom TGA Writer Logic
    if (outputPath.endsWith(".tga", Qt::CaseInsensitive)) {
        QImage saveImg = resultImg;
        
        // Ensure format is ARGB32 (non-premultiplied)
        if (saveImg.format() != QImage::Format_ARGB32) {
             saveImg = saveImg.convertToFormat(QImage::Format_ARGB32);
        }

        // [Requirement] Export to PNG then to TGA
        // This acts as a sanitization step requested by user
        QByteArray pngData;
        QBuffer buffer(&pngData);
        buffer.open(QIODevice::WriteOnly);
        if (saveImg.save(&buffer, "PNG")) {
            QImage pngSanitized;
            pngSanitized.loadFromData(pngData, "PNG");
            if (!pngSanitized.isNull()) {
                saveImg = pngSanitized;
                if (saveImg.format() != QImage::Format_ARGB32) {
                     saveImg = saveImg.convertToFormat(QImage::Format_ARGB32);
                }
            }
        }
        
        // writeTGA is defined at the top of this file
        // Also apply alpha inversion if requested
        if (invertTgaAlpha) {
            // Invert Alpha for the output image
            int w = saveImg.width();
            int h = saveImg.height();
            for (int y = 0; y < h; ++y) {
                quint32* line = reinterpret_cast<quint32*>(saveImg.scanLine(y));
                for (int x = 0; x < w; ++x) {
                    line[x] ^= 0xFF000000;
                }
            }
        }

        if (writeTGA(saveImg, outputPath)) {
            return QString();
        } else {
             return "写入 TGA 文件失败: " + outputPath;
        }

    } else if (outputPath.endsWith(".blp", Qt::CaseInsensitive)) {
        // Use custom BLP Writer
         QImage saveImg = resultImg;
         BlpReader blpWriter;
         if (blpWriter.write(saveImg, outputPath)) {
             return QString();
         } else {
             return "写入 BLP 文件失败: " + outputPath;
         }

    } else {
        // Standard save for other formats
        if (resultImg.save(outputPath)) {
            return QString();
        } else {
            return "写入文件失败: " + outputPath;
        }
    }
}
