#pragma once

#include <QString>
#include <QImage>

class ImageProcessor
{
public:
    ImageProcessor();
    
    // Loads the mask into memory
    bool loadMask(const QString& maskPath, bool invertAlpha = false);

    // Processes a single image file
    // Returns empty string if success, error message otherwisew
    // blendMode: compatible with QPainter::CompositionMode or custom logic
    // 0: Alpha (DestinationIn), 1: Normal, 2: Multiply, ... etc
    // invertTgaAlpha: Invert Alpha channel for TGA output/input (for some games)
    QString processImage(const QString& inputPath, const QString& outputPath, int blendMode, bool invertTgaAlpha);

    static QImage loadTGA(const QString& path, bool invertAlpha = false); // Custom TGA Loader

private:
    QImage m_maskImage;
    
    // Helper to load image texturing specifically dealing with BLP if needed
    QImage loadImageHelper(const QString& path, bool invertAlpha = false);
};
