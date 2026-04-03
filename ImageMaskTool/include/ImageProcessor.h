#pragma once

#include <QString>
#include <QImage>

class ImageProcessor
{
public:
    enum OutputSizeMode {
        OutputSizeMask = 0,
        OutputSizeInput = 1,
        OutputSizeBase = 2
    };

    enum OriginalTileMode {
        OriginalStretch = 0,
        OriginalRepeat = 1,
        OriginalMirror = 2,
        OriginalCenter = 3,
        OriginalClamp = 4
    };

    enum OriginalScaleMode {
        ScalePercent = 0,
        ScaleAutoFit = 1,
        ScaleOneToOne = 2
    };

    enum OriginalAnchorMode {
        AnchorCenter = 0,
        AnchorTopLeft = 1,
        AnchorTopRight = 2,
        AnchorBottomLeft = 3,
        AnchorBottomRight = 4
    };

    enum OriginalDirectionMode {
        DirectionXY = 0,
        DirectionX = 1,
        DirectionY = 2
    };

    enum OriginalEdgeMode {
        EdgeCrop = 0,
        EdgeTransparent = 1,
        EdgeFeather = 2
    };

    enum OriginalQualityMode {
        QualitySmooth = 0,
        QualityFast = 1
    };

    ImageProcessor();
    
    // Loads the mask into memory
    bool loadMask(const QString& maskPath, bool invertAlpha = false);

    // Loads the base image into memory
    bool loadBase(const QString& basePath, bool invertAlpha = false);

    // Processes a single image file
    // Returns empty string if success, error message otherwisew
    // blendMode: compatible with QPainter::CompositionMode or custom logic
    // 0: Alpha (DestinationIn), 1: Normal, 2: Multiply, ... etc
    // invertTgaAlpha: Invert Alpha channel for TGA output/input (for some games)
    QString processImage(const QString& inputPath, const QString& outputPath, int blendMode, bool invertTgaAlpha,
                         int outputSizeMode, int originalTileMode, int originalScaleMode,
                         double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                         int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                         int originalQualityMode);

    // Generate processed result in memory for preview.
    QString processImagePreview(const QString& inputPath, int blendMode, bool invertTgaAlpha,
                                int outputSizeMode, int originalTileMode, int originalScaleMode,
                                double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                                int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                                int originalQualityMode, QImage& outImage);

    static QImage loadTGA(const QString& path, bool invertAlpha = false); // Custom TGA Loader

private:
    QImage m_maskImage;
    QImage m_baseImage;
    
    // Helper to load image texturing specifically dealing with BLP if needed
    QImage loadImageHelper(const QString& path, bool invertAlpha = false);

    QString processImageInternal(const QString& inputPath, int blendMode, bool invertTgaAlpha,
                                 int outputSizeMode, int originalTileMode, int originalScaleMode,
                                 double originalScalePercent, int originalAnchorMode, int originalOffsetX,
                                 int originalOffsetY, int originalDirectionMode, int originalEdgeMode,
                                 int originalQualityMode, QImage& outImage);
};
