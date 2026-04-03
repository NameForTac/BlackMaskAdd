#include "ImageProcessor.h"
#include "BlpReader.h"
#include <QPainter>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <algorithm>
#include <cmath>

namespace {

QPoint centeredTopLeft(const QSize& canvasSize, const QSize& imageSize)
{
    return QPoint((canvasSize.width() - imageSize.width()) / 2,
                  (canvasSize.height() - imageSize.height()) / 2);
}

QPoint anchoredTopLeft(const QSize& canvasSize, const QSize& imageSize, int anchorMode)
{
    switch (anchorMode) {
        case ImageProcessor::AnchorTopLeft:
            return QPoint(0, 0);
        case ImageProcessor::AnchorTopRight:
            return QPoint(canvasSize.width() - imageSize.width(), 0);
        case ImageProcessor::AnchorBottomLeft:
            return QPoint(0, canvasSize.height() - imageSize.height());
        case ImageProcessor::AnchorBottomRight:
            return QPoint(canvasSize.width() - imageSize.width(), canvasSize.height() - imageSize.height());
        case ImageProcessor::AnchorCenter:
        default:
            return centeredTopLeft(canvasSize, imageSize);
    }
}

QImage scaledByMode(const QImage& img, const QSize& canvasSize, int scaleMode, double scalePercent, bool smooth)
{
    if (img.isNull()) {
        return img;
    }

    double factor = 1.0;
    if (scaleMode == ImageProcessor::ScaleAutoFit) {
        const double sx = static_cast<double>(canvasSize.width()) / std::max(1, img.width());
        const double sy = static_cast<double>(canvasSize.height()) / std::max(1, img.height());
        factor = std::max(sx, sy);
    } else if (scaleMode == ImageProcessor::ScaleOneToOne) {
        factor = 1.0;
    } else {
        factor = std::max(0.01, scalePercent / 100.0);
    }

    const int w = std::max(1, static_cast<int>(std::round(img.width() * factor)));
    const int h = std::max(1, static_cast<int>(std::round(img.height() * factor)));
    return img.scaled(w, h, Qt::IgnoreAspectRatio, smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
}

void drawTiledLayer(QPainter& painter, const QImage& tileImg, const QSize& canvasSize,
                    int anchorMode, int offsetX, int offsetY, int directionMode,
                    bool mirror, bool smooth)
{
    if (tileImg.isNull() || tileImg.width() <= 0 || tileImg.height() <= 0) {
        return;
    }

    QImage base = tileImg;
    if (!smooth) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    }

    const QPoint anchor = anchoredTopLeft(canvasSize, base.size(), anchorMode) + QPoint(offsetX, offsetY);
    const int w = base.width();
    const int h = base.height();

    const int startX = anchor.x() - static_cast<int>(std::ceil(static_cast<double>(anchor.x()) / w)) * w;
    const int startY = anchor.y() - static_cast<int>(std::ceil(static_cast<double>(anchor.y()) / h)) * h;

    const int loopStartX = (directionMode == ImageProcessor::DirectionY) ? anchor.x() : startX;
    const int loopEndX = (directionMode == ImageProcessor::DirectionY) ? (anchor.x() + 1) : canvasSize.width();
    const int loopStepX = (directionMode == ImageProcessor::DirectionY) ? canvasSize.width() + 1 : w;

    const int loopStartY = (directionMode == ImageProcessor::DirectionX) ? anchor.y() : startY;
    const int loopEndY = (directionMode == ImageProcessor::DirectionX) ? (anchor.y() + 1) : canvasSize.height();
    const int loopStepY = (directionMode == ImageProcessor::DirectionX) ? canvasSize.height() + 1 : h;

    QImage flipH = base.flipped(Qt::Horizontal);
    QImage flipV = base.flipped(Qt::Vertical);
    QImage flipHV = base.flipped(Qt::Horizontal | Qt::Vertical);

    for (int y = loopStartY; y < loopEndY; y += loopStepY) {
        for (int x = loopStartX; x < loopEndX; x += loopStepX) {
            if (!mirror) {
                painter.drawImage(x, y, base);
                continue;
            }

            const int tx = static_cast<int>(std::floor(static_cast<double>(x - startX) / w));
            const int ty = static_cast<int>(std::floor(static_cast<double>(y - startY) / h));
            const bool oddX = (tx & 1) != 0;
            const bool oddY = (ty & 1) != 0;

            if (oddX && oddY) {
                painter.drawImage(x, y, flipHV);
            } else if (oddX) {
                painter.drawImage(x, y, flipH);
            } else if (oddY) {
                painter.drawImage(x, y, flipV);
            } else {
                painter.drawImage(x, y, base);
            }
        }
    }
}

void drawClampLayer(QPainter& painter, const QImage& img, const QSize& canvasSize,
                    int anchorMode, int offsetX, int offsetY, bool smooth)
{
    if (img.isNull()) {
        return;
    }

    QImage source = img;
    if (!smooth) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    }

    const QPoint topLeft = anchoredTopLeft(canvasSize, source.size(), anchorMode) + QPoint(offsetX, offsetY);
    const QRect canvasRect(QPoint(0, 0), canvasSize);
    const QRect srcRect(topLeft, source.size());

    painter.drawImage(topLeft, source);

    if (!canvasRect.intersects(srcRect)) {
        return;
    }

    if (topLeft.x() > 0) {
        QImage leftStrip = source.copy(0, 0, 1, source.height());
        painter.drawImage(QRect(0, topLeft.y(), topLeft.x(), source.height()), leftStrip);
    }
    if (srcRect.right() < canvasRect.right()) {
        QImage rightStrip = source.copy(source.width() - 1, 0, 1, source.height());
        painter.drawImage(QRect(srcRect.right() + 1, topLeft.y(), canvasRect.right() - srcRect.right(), source.height()), rightStrip);
    }
    if (topLeft.y() > 0) {
        QImage topStrip = source.copy(0, 0, source.width(), 1);
        painter.drawImage(QRect(topLeft.x(), 0, source.width(), topLeft.y()), topStrip);
    }
    if (srcRect.bottom() < canvasRect.bottom()) {
        QImage bottomStrip = source.copy(0, source.height() - 1, source.width(), 1);
        painter.drawImage(QRect(topLeft.x(), srcRect.bottom() + 1, source.width(), canvasRect.bottom() - srcRect.bottom()), bottomStrip);
    }
}

void drawOriginalLayer(QPainter& painter, const QImage& img, const QSize& canvasSize,
                       int tileMode, int scaleMode, double scalePercent, int anchorMode,
                       int offsetX, int offsetY, int directionMode, int edgeMode,
                       int qualityMode)
{
    if (img.isNull()) {
        return;
    }

    const bool smooth = (qualityMode == ImageProcessor::QualitySmooth);

    if (tileMode == ImageProcessor::OriginalStretch) {
        if (edgeMode == ImageProcessor::EdgeTransparent) {
            QImage fit = img.scaled(canvasSize, Qt::KeepAspectRatio, smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
            QPoint topLeft = anchoredTopLeft(canvasSize, fit.size(), anchorMode) + QPoint(offsetX, offsetY);
            painter.drawImage(topLeft, fit);
        } else {
            QImage stretched = img;
            if (img.size() != canvasSize) {
                stretched = img.scaled(canvasSize, Qt::IgnoreAspectRatio, smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
            }
            painter.drawImage(0, 0, stretched);
        }
        return;
    }

    QImage scaled = scaledByMode(img, canvasSize, scaleMode, scalePercent, smooth);

    if (tileMode == ImageProcessor::OriginalCenter) {
        QPoint topLeft = anchoredTopLeft(canvasSize, scaled.size(), anchorMode) + QPoint(offsetX, offsetY);
        painter.drawImage(topLeft, scaled);
    } else if (tileMode == ImageProcessor::OriginalClamp) {
        drawClampLayer(painter, scaled, canvasSize, anchorMode, offsetX, offsetY, smooth);
    } else if (tileMode == ImageProcessor::OriginalMirror) {
        drawTiledLayer(painter, scaled, canvasSize, anchorMode, offsetX, offsetY, directionMode, true, smooth);
    } else {
        drawTiledLayer(painter, scaled, canvasSize, anchorMode, offsetX, offsetY, directionMode, false, smooth);
    }

    if (edgeMode == ImageProcessor::EdgeFeather) {
        const int featherPx = 4;
        QImage alphaMask(canvasSize, QImage::Format_ARGB32_Premultiplied);
        alphaMask.fill(Qt::transparent);
        QPainter maskPainter(&alphaMask);
        for (int y = 0; y < canvasSize.height(); ++y) {
            for (int x = 0; x < canvasSize.width(); ++x) {
                int edgeDist = std::min(std::min(x, y), std::min(canvasSize.width() - 1 - x, canvasSize.height() - 1 - y));
                int a = (edgeDist >= featherPx) ? 255 : (edgeDist * 255 / std::max(1, featherPx));
                maskPainter.setPen(QColor(255, 255, 255, a));
                maskPainter.drawPoint(x, y);
            }
        }
        maskPainter.end();

        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.drawImage(0, 0, alphaMask);
    }
}

void drawStretchLayer(QPainter& painter, const QImage& img, const QSize& canvasSize)
{
    if (img.isNull()) {
        return;
    }

    if (img.size() == canvasSize) {
        painter.drawImage(0, 0, img);
        return;
    }

    painter.drawImage(0, 0, img.scaled(canvasSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

}

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

QString ImageProcessor::processImageInternal(const QString& inputPath, int blendMode, bool invertTgaAlpha,
                                            int outputSizeMode, int originalTileMode, int originalScaleMode,
                                            double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                                            int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                                            int originalQualityMode, QImage& outImage)
{
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

    if (inputImg.format() != QImage::Format_ARGB32_Premultiplied) {
        inputImg = inputImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QImage baseImg = m_baseImage;
    if (baseImg.format() != QImage::Format_ARGB32_Premultiplied) {
        baseImg = baseImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QImage scaledMask = m_maskImage;
    if (scaledMask.format() != QImage::Format_ARGB32) {
        scaledMask = scaledMask.convertToFormat(QImage::Format_ARGB32);
    }

    if (!scaledMask.hasAlphaChannel()) {
        QImage alphaChannelMask = scaledMask.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < alphaChannelMask.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(alphaChannelMask.scanLine(y));
            for (int x = 0; x < alphaChannelMask.width(); ++x) {
                int alpha = qGray(line[x]);
                line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), alpha);
            }
        }
        scaledMask = alphaChannelMask;
    }
    if (scaledMask.format() != QImage::Format_ARGB32_Premultiplied) {
        scaledMask = scaledMask.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QSize outputSize = inputImg.size();
    if (outputSizeMode == OutputSizeMask) {
        outputSize = scaledMask.size();
    } else if (outputSizeMode == OutputSizeBase) {
        outputSize = baseImg.size();
    }

    if (!outputSize.isValid()) {
        return "输出尺寸无效。";
    }

    QImage resultImg(outputSize, QImage::Format_ARGB32_Premultiplied);
    resultImg.fill(Qt::transparent);

    QPainter painter(&resultImg);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawStretchLayer(painter, baseImg, outputSize);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawOriginalLayer(painter, inputImg, outputSize,
                      originalTileMode, originalScaleMode, originalScalePercent,
                      originalAnchorMode, originalOffsetX, originalOffsetY,
                      originalDirectionMode, originalEdgeMode, originalQualityMode);

    if (blendMode == 0) {
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        drawStretchLayer(painter, scaledMask, outputSize);
    } else {
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
        drawStretchLayer(painter, scaledMask, outputSize);
    }

    painter.end();
    outImage = resultImg;

    return QString();
}

QString ImageProcessor::processImagePreview(const QString& inputPath, int blendMode, bool invertTgaAlpha,
                                            int outputSizeMode, int originalTileMode, int originalScaleMode,
                                            double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                                            int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                                            int originalQualityMode, QImage& outImage)
{
    return processImageInternal(inputPath, blendMode, invertTgaAlpha,
                                outputSizeMode, originalTileMode, originalScaleMode,
                                originalScalePercent, originalAnchorMode, originalOffsetX,
                                originalOffsetY, originalDirectionMode, originalEdgeMode,
                                originalQualityMode, outImage);
}

QString ImageProcessor::processImage(const QString& inputPath, const QString& outputPath, int blendMode, bool invertTgaAlpha,
                                    int outputSizeMode, int originalTileMode, int originalScaleMode,
                                    double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                                    int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                                    int originalQualityMode)
{
    QImage resultImg;
    QString processError = processImageInternal(inputPath, blendMode, invertTgaAlpha,
                                                outputSizeMode, originalTileMode, originalScaleMode,
                                                originalScalePercent, originalAnchorMode, originalOffsetX,
                                                originalOffsetY, originalDirectionMode, originalEdgeMode,
                                                originalQualityMode, resultImg);
    if (!processError.isEmpty()) {
        return processError;
    }

    if (outputPath.endsWith(".tga", Qt::CaseInsensitive)) {
        QImage saveImg = resultImg;

        if (saveImg.format() != QImage::Format_ARGB32) {
             saveImg = saveImg.convertToFormat(QImage::Format_ARGB32);
        }

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

        if (invertTgaAlpha) {
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
         QImage saveImg = resultImg;
         BlpReader blpWriter;
         if (blpWriter.write(saveImg, outputPath)) {
             return QString();
         } else {
             return "写入 BLP 文件失败: " + outputPath;
         }

    } else {
        if (resultImg.save(outputPath)) {
            return QString();
        } else {
            return "写入文件失败: " + outputPath;
        }
    }
}
