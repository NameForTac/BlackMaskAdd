#include "MainWindow.h"
#include "ImageProcessor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDebug>
#include <QPainter>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    // Apply Stylized QSS
    QString primaryColor = "#409EFF";
    QString bgColor = "#F6F8FA";
    QString cardColor = "#FFFFFF";
    QString textColor = "#303133";
    QString borderColor = "#DCDFE6";

    // Global Stylesheet
    this->setStyleSheet(QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { font-family: 'Microsoft YaHei UI', 'Segoe UI', sans-serif; font-size: 14px; color: %2; }"
        
        // Card Frame Style
        "QFrame#Card { background-color: %3; border-radius: 8px; border: 1px solid #E4E7ED; }"
        
        // Label Style
        "QLabel { font-weight: 500; color: #606266; }"
        "QLabel#CardTitle { font-size: 16px; font-weight: bold; color: #303133; padding-bottom: 8px; border-bottom: 1px solid #EBEEF5; margin-bottom: 8px; }"
        
        // Inputs
        "QLineEdit { border: 1px solid %4; border-radius: 4px; padding: 6px; background-color: #FFFFFF; selection-background-color: %5; }"
        "QLineEdit:focus { border-color: %5; }"
        "QComboBox { border: 1px solid %4; border-radius: 4px; padding: 6px; background-color: #FFFFFF; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        
        // Buttons
        "QPushButton { background-color: #FFFFFF; border: 1px solid %4; border-radius: 4px; padding: 6px 12px; color: #606266; }"
        "QPushButton:hover { background-color: #ECF5FF; color: %5; border-color: #C6E2FF; }"
        "QPushButton:pressed { background-color: #D9ECFF; border-color: %5; }"
        
        "QPushButton#PrimaryButton { background-color: %5; color: white; border: none; font-weight: bold; font-size: 15px; }"
        "QPushButton#PrimaryButton:hover { background-color: #66B1FF; }"
        "QPushButton#PrimaryButton:pressed { background-color: #3A8EE6; }"
        "QPushButton#PrimaryButton:disabled { background-color: #A0CFFF; color: #FFFFFF; }"

        // Checkbox
        "QCheckBox { spacing: 8px; color: #606266; }"
        "QCheckBox::indicator { width: 18px; height: 18px; margin-right: 4px; border: 1px solid %4; border-radius: 3px; background-color: white; }"
        "QCheckBox::indicator:checked { background-color: %5; border-color: %5; image: url(:/icons/check.png); }" // Assuming no icon yet, just color
        
        // Progress Bar
        "QProgressBar { border: none; background-color: #EBEEF5; border-radius: 6px; text-align: center; color: transparent; }"
        "QProgressBar::chunk { background-color: %5; border-radius: 6px; }"
        
        // Log Area
        "QTextEdit { border: 1px solid %4; border-radius: 6px; background-color: #FAFAFA; font-family: Consolas, monospace; font-size: 12px; color: #555; }"
    ).arg(bgColor, textColor, cardColor, borderColor, primaryColor));

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Main Layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // --- Header ---
    QLabel* headerLabel = new QLabel("图片批量遮罩工具");
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133; margin-bottom: 4px;");
    mainLayout->addWidget(headerLabel);

    // --- Content Area (Grid) ---
    QGridLayout* contentGrid = new QGridLayout();
    contentGrid->setSpacing(16);

    // 1. CARD: Source Settings (Input & Mask)
    // Span across 2 columns
    QFrame* sourceCard = new QFrame();
    sourceCard->setObjectName("Card");
    QGridLayout* sourceLayout = new QGridLayout(sourceCard);
    sourceLayout->setContentsMargins(16, 16, 16, 16);
    sourceLayout->setSpacing(12);
    
    QLabel* sourceTitle = new QLabel("资源设置");
    sourceTitle->setObjectName("CardTitle");
    sourceLayout->addWidget(sourceTitle, 0, 0, 1, 3);

    // Input Folder Row
    QLabel* inputLabel = new QLabel("输入文件夹:");
    inputLabel->setFixedWidth(90);
    m_inputFolderEdit = new QLineEdit();
    m_inputFolderEdit->setPlaceholderText("拖入文件夹或点击浏览...");
    QPushButton* browseInputBtn = new QPushButton("浏览");
    connect(browseInputBtn, &QPushButton::clicked, this, &MainWindow::browseInputFolder);
    sourceLayout->addWidget(inputLabel, 1, 0);
    sourceLayout->addWidget(m_inputFolderEdit, 1, 1);
    sourceLayout->addWidget(browseInputBtn, 1, 2);

    // Output Folder Row
    QLabel* outputLabel = new QLabel("输出文件夹:");
    outputLabel->setFixedWidth(90);
    m_outputFolderEdit = new QLineEdit();
    m_outputFolderEdit->setPlaceholderText("留空则默认与输入文件夹一致...");
    QPushButton* browseOutputBtn = new QPushButton("浏览");
    connect(browseOutputBtn, &QPushButton::clicked, this, &MainWindow::browseOutputFolder);
    sourceLayout->addWidget(outputLabel, 2, 0);
    sourceLayout->addWidget(m_outputFolderEdit, 2, 1);
    sourceLayout->addWidget(browseOutputBtn, 2, 2);

    // Mask File Row
    QLabel* maskLabel = new QLabel("遮罩文件:");
    maskLabel->setFixedWidth(90);
    m_maskFileEdit = new QLineEdit();
    m_maskFileEdit->setPlaceholderText("拖入 mask.png 图片...");
    connect(m_maskFileEdit, &QLineEdit::textChanged, this, &MainWindow::updatePreview);
    QPushButton* browseMaskBtn = new QPushButton("浏览");
    connect(browseMaskBtn, &QPushButton::clicked, this, &MainWindow::browseMaskFile);
    sourceLayout->addWidget(maskLabel, 3, 0);
    sourceLayout->addWidget(m_maskFileEdit, 3, 1);
    sourceLayout->addWidget(browseMaskBtn, 3, 2);

    contentGrid->addWidget(sourceCard, 0, 0, 1, 2); // Row 0, Span 2 cols

    // 2. CARD: Preview (New Card)
    QFrame* previewCard = new QFrame();
    previewCard->setObjectName("Card");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewCard);
    previewLayout->setContentsMargins(16, 16, 16, 16);
    
    QLabel* previewTitle = new QLabel("效果预览");
    previewTitle->setObjectName("CardTitle");
    previewLayout->addWidget(previewTitle);

    QHBoxLayout* previewImagesLayout = new QHBoxLayout();
    previewImagesLayout->setSpacing(8);
    
    auto createPreviewLabel = [](const QString& title) -> QLabel* {
        QLabel* label = new QLabel();
        label->setFixedSize(128, 128); // Thumbnail size
        label->setStyleSheet("border: 1px dashed #DCDFE6; border-radius: 4px; background-color: #FAFAFA;");
        label->setAlignment(Qt::AlignCenter);
        label->setText(title);
        return label;
    };

    QVBoxLayout* p1 = new QVBoxLayout(); 
    p1->addWidget(new QLabel("原图 (Sample)"), 0, Qt::AlignCenter);
    m_previewOriginal = createPreviewLabel("等待载入...");
    p1->addWidget(m_previewOriginal, 0, Qt::AlignCenter);
    
    QVBoxLayout* p2 = new QVBoxLayout();
    p2->addWidget(new QLabel("遮罩 (Mask)"), 0, Qt::AlignCenter);
    m_previewMask = createPreviewLabel("等待载入...");
    p2->addWidget(m_previewMask, 0, Qt::AlignCenter);

    QVBoxLayout* p3 = new QVBoxLayout();
    p3->addWidget(new QLabel("结果 (Result)"), 0, Qt::AlignCenter);
    m_previewResult = createPreviewLabel("等待处理...");
    p3->addWidget(m_previewResult, 0, Qt::AlignCenter);

    previewImagesLayout->addLayout(p1);
    // Add arrow or symbol?
    previewImagesLayout->addWidget(new QLabel("+"), 0, Qt::AlignCenter);
    previewImagesLayout->addLayout(p2);
    previewImagesLayout->addWidget(new QLabel("="), 0, Qt::AlignCenter);
    previewImagesLayout->addLayout(p3);

    previewLayout->addLayout(previewImagesLayout);
    contentGrid->addWidget(previewCard, 1, 0, 1, 2); // Row 1, Span 2 cols


    // 3. CARD: Output Configuration (Prefix/Suffix/Format)
    QFrame* configCard = new QFrame();
    configCard->setObjectName("Card");
    QGridLayout* configLayout = new QGridLayout(configCard);
    configLayout->setContentsMargins(16, 16, 16, 16);
    configLayout->setVerticalSpacing(12);
    
    QLabel* configTitle = new QLabel("输出规则"); // Naming Rule
    configTitle->setObjectName("CardTitle");
    configLayout->addWidget(configTitle, 0, 0, 1, 2);

    // Prefix
    configLayout->addWidget(new QLabel("前缀:"), 1, 0);
    m_prefixEdit = new QLineEdit();
    m_prefixEdit->setPlaceholderText("(可选)");
    configLayout->addWidget(m_prefixEdit, 1, 1);

    // Suffix
    configLayout->addWidget(new QLabel("后缀:"), 2, 0);
    m_suffixEdit = new QLineEdit(); // Default empty
    m_suffixEdit->setPlaceholderText("(可选)");
    configLayout->addWidget(m_suffixEdit, 2, 1);

    // Format
    configLayout->addWidget(new QLabel("格式:"), 3, 0);
    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItem("PNG - Portable Network Graphics"); 
    m_outputFormatCombo->addItem("TGA - Truevision TGA");
    m_outputFormatCombo->addItem("JPG - JPEG Image");
    m_outputFormatCombo->addItem("BLP - Blizzard Texture");
    configLayout->addWidget(m_outputFormatCombo, 3, 1);

    contentGrid->addWidget(configCard, 2, 0); // Row 2, Col 0

    // 4. CARD: Processing Options (Blend Mode / TGA Fix)
    QFrame* optionsCard = new QFrame();
    optionsCard->setObjectName("Card");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsCard);
    optionsLayout->setContentsMargins(16, 16, 16, 16);
    optionsLayout->setSpacing(12);
    
    QLabel* optionsTitle = new QLabel("图像算法");
    optionsTitle->setObjectName("CardTitle");
    optionsLayout->addWidget(optionsTitle);

    // Blend Mode
    QHBoxLayout* blendLayout = new QHBoxLayout();
    blendLayout->addWidget(new QLabel("混合模式:"));
    m_blendModeCombo = new QComboBox();
    m_blendModeCombo->addItem("Alpha Mask (遮罩透明)"); // 0
    m_blendModeCombo->addItem("Normal (覆盖)");        // 1
    m_blendModeCombo->addItem("Multiply (正片叠底)");  // 2
    m_blendModeCombo->addItem("Screen (滤色)");        // 3
    m_blendModeCombo->addItem("Overlay (叠加)");       // 4
    m_blendModeCombo->addItem("Darken (变暗)");        // 5
    m_blendModeCombo->addItem("Lighten (变亮)");       // 6
    m_blendModeCombo->addItem("Add (添加)");           // 7
    m_blendModeCombo->setCurrentIndex(0);
    connect(m_blendModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    blendLayout->addWidget(m_blendModeCombo);
    optionsLayout->addLayout(blendLayout);

    optionsLayout->addStretch(); // Push checkbox down slightly

    // TGA Fix
    m_tgaAlphaInvertCheckbox = new QCheckBox("反转 TGA Alpha 通道 (魔兽兼容修复)");
    connect(m_tgaAlphaInvertCheckbox, &QCheckBox::checkStateChanged, this, &MainWindow::updatePreview);
    m_tgaAlphaInvertCheckbox->setToolTip("如果是为魔兽争霸3制作贴图，建议勾选此项以修复透明度反转问题。");
    optionsLayout->addWidget(m_tgaAlphaInvertCheckbox);

    contentGrid->addWidget(optionsCard, 2, 1); // Row 2, Col 1

    // Add Main Grid to Layout
    mainLayout->addLayout(contentGrid);

    // --- Action Bar ---
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    m_startBtn = new QPushButton("开始处理");
    m_startBtn->setObjectName("PrimaryButton");
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setFixedHeight(45);
    m_startBtn->setMinimumWidth(150);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::startProcessing);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(12);
    m_progressBar->setTextVisible(false); // Clean look

    actionLayout->addWidget(m_startBtn);
    actionLayout->addSpacing(16);
    QVBoxLayout* progLayout = new QVBoxLayout(); // Wrap progress bar to align center vertically
    progLayout->addWidget(new QLabel("处理进度:"));
    progLayout->addWidget(m_progressBar);
    actionLayout->addLayout(progLayout);

    mainLayout->addLayout(actionLayout);

    // --- Log Area ---
    // Log wrapped in a simple frame? Or just clean TextEdit
    m_logArea = new QTextEdit();
    m_logArea->setReadOnly(true);
    m_logArea->setPlaceholderText("等待任务开始...");
    mainLayout->addWidget(m_logArea);

    // Window Settings
    resize(800, 650); // Slightly larger for card layout
    setWindowTitle("图片批量遮罩工具By某某人"); // Simplify title

    // Debugging info (System Diagnostics)
    log("=== 系统环境诊断 ===");
    QString appPath = QCoreApplication::applicationDirPath();
    log("运行路径: " + appPath);
    log("库路径 (Library Paths): " + QCoreApplication::libraryPaths().join("; "));
    
    QDir pluginsDir(appPath + "/imageformats");
    if (pluginsDir.exists()) {
        log("检测到 imageformats 目录，包含文件: " + pluginsDir.entryList(QDir::Files).join(", "));
    } else {
        log("警告: imageformats 目录不存在！");
    }

    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QStringList formatStrs;
    for (auto fmt : formats) formatStrs << QString(fmt);
    log("支持写出的格式: " + formatStrs.join(", "));

    if (!formatStrs.contains("tga", Qt::CaseInsensitive)) {
        log("警告: TGA 插件未加载，无法输出 .tga 文件！");
    } else {
        log("TGA 插件已就绪。");
    }
    log("====================");
}

void MainWindow::browseInputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择输入文件夹");
    if (!dir.isEmpty()) {
        m_inputFolderEdit->setText(dir);
    }
}

void MainWindow::browseMaskFile()
{
    QString file = QFileDialog::getOpenFileName(this, "选择遮罩文件", QString(), "Images (*.png *.jpg *.bmp *.tga *.blp)");
    if (!file.isEmpty()) {
        m_maskFileEdit->setText(file);
    }
}

void MainWindow::log(const QString& message)
{
    m_logArea->append(message);
}


void MainWindow::browseOutputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择输出文件夹");
    if (!dir.isEmpty()) {
        m_outputFolderEdit->setText(dir);
    }
}

void MainWindow::updateSampleImage()
{
    // Try to find a valid image in the input folder
    QString folderPath = m_inputFolderEdit->text();
    if (folderPath.isEmpty() || !QDir(folderPath).exists()) return;

    QDir dir(folderPath);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tga" << "*.blp";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    if (!fileList.isEmpty()) {
        m_sampleImagePath = fileList.first().absoluteFilePath();
        updatePreview();
    }
}

void MainWindow::updatePreview()
{
    // Check requirements
    if (m_sampleImagePath.isEmpty()) {
        m_previewOriginal->setText("无样本");
        m_previewResult->setText("请选择\n输入文件夹");
        return;
    }
    
    QString maskPath = m_maskFileEdit->text();
    if (maskPath.isEmpty() || !QFile::exists(maskPath)) {
        m_previewMask->setText("无遮罩");
        m_previewResult->setText("请选择\n遮罩文件");
        // Load original anyway
        ImageProcessor proc;
        QImage orig = ImageProcessor::loadTGA(m_sampleImagePath, false); // Static call or need instance?
        // Actually loadTGA handles TGA but we need generic loader
        // Let's use QImageReader for generic
        if (orig.isNull()) orig.load(m_sampleImagePath);
        
        if (!orig.isNull()) {
             m_previewOriginal->setPixmap(QPixmap::fromImage(orig.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        } else {
             m_previewOriginal->setText("加载失败");
        }
        return;
    }

    int blendMode = m_blendModeCombo->currentIndex();
    bool invertTga = m_tgaAlphaInvertCheckbox->isChecked();

    // Run preview generation in background to avoid freezing UI
    (void)QtConcurrent::run([=]() {
        ImageProcessor processor;
        if (!processor.loadMask(maskPath, invertTga)) {
             QMetaObject::invokeMethod(this, [=]() { m_previewMask->setText("加载失败"); });
             return;
        }

        // We can't access m_maskImage directly easily since it's private and we are inside MainWindow.
        // But processor has it loaded.
        // We need a way to get the processed image without saving to file.
        // Update: ImageProcessor::processImage saves to file. We need a memory version.
        // Let's modify ImageProcessor slightly or just use processImage with a temp file? 
        // Better: refactor ImageProcessor to return QImage.
        // For now, let's duplicate logic quickly or rely on a new method.
        
        // Since I can't easily change the API without reading ImageProcessor.h again,
        // I will implement a "preview" logic here duplicating some steps or use a temp file.
        // Actually, ImageProcessor process logic is largely in processImage.
        // Let's stick to simple preview logic here for now to avoid breaking core.
        
        // Load Input
        QImage inputImg;
        if (m_sampleImagePath.endsWith(".blp", Qt::CaseInsensitive)) {
             // Need BlpReader? MainWindow includes ImageProcessor.h but not BlpReader.h directly maybe?
             // ImageProcessor.cpp includes it.
             // Ideally ImageProcessor should expose "load" and "processInMemory".
             // For now, let's just use QImage::load handling standard formats. 
             // If BLP, preview might fail if no plugin.
             inputImg.load(m_sampleImagePath);
        } else if (m_sampleImagePath.endsWith(".tga", Qt::CaseInsensitive)) {
             inputImg = ImageProcessor::loadTGA(m_sampleImagePath, invertTga);
        } else {
             inputImg.load(m_sampleImagePath);
        }

        if (inputImg.isNull()) return;

        // Load Mask (for visualization)
        QImage maskImg;
        if (maskPath.endsWith(".tga", Qt::CaseInsensitive)) {
            maskImg = ImageProcessor::loadTGA(maskPath, invertTga); // apply invert if TGA
        } else {
            maskImg.load(maskPath);
        }
        
        // Process
        if (inputImg.format() != QImage::Format_ARGB32_Premultiplied)
            inputImg = inputImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QImage scaledMask = maskImg.scaled(inputImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        // Generate alpha from grayscale if needed (Logic copy from ImageProcessor)
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

        QImage resultImg = inputImg;
        QPainter painter(&resultImg);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        if (blendMode == 0) { // Alpha Mask
             painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
             painter.drawImage(0, 0, scaledMask);
        } else {
             // ... Map blend modes
             switch (blendMode) {
                case 1: painter.setCompositionMode(QPainter::CompositionMode_SourceOver); break;
                case 2: painter.setCompositionMode(QPainter::CompositionMode_Multiply); break;
                case 3: painter.setCompositionMode(QPainter::CompositionMode_Screen); break;
                case 4: painter.setCompositionMode(QPainter::CompositionMode_Overlay); break;
                case 5: painter.setCompositionMode(QPainter::CompositionMode_Darken); break;
                case 6: painter.setCompositionMode(QPainter::CompositionMode_Lighten); break;
                case 7: painter.setCompositionMode(QPainter::CompositionMode_Plus); break;
                default: painter.setCompositionMode(QPainter::CompositionMode_SourceOver); break;
             }
             if (scaledMask.format() != QImage::Format_ARGB32_Premultiplied) {
                 scaledMask = scaledMask.convertToFormat(QImage::Format_ARGB32_Premultiplied);
             }
             painter.drawImage(0, 0, scaledMask);
        }
        painter.end();

        // Update UI
        QMetaObject::invokeMethod(this, [=]() {
            m_previewOriginal->setPixmap(QPixmap::fromImage(inputImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            m_previewMask->setPixmap(QPixmap::fromImage(maskImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            m_previewResult->setPixmap(QPixmap::fromImage(resultImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        });

    });
}

void MainWindow::startProcessing(void)
{
    QString folderPath = m_inputFolderEdit->text();
    QString outputFolder = m_outputFolderEdit->text(); // New
    QString maskPath = m_maskFileEdit->text();
    QString prefix = m_prefixEdit->text();
    QString suffix = m_suffixEdit->text();
    int formatIndex = m_outputFormatCombo->currentIndex();
    int blendMode = m_blendModeCombo->currentIndex();
    bool invertTga = m_tgaAlphaInvertCheckbox->isChecked();

    if (folderPath.isEmpty() || !QDir(folderPath).exists()) {
        QMessageBox::warning(this, "错误", "无效的输入文件夹");
        return;
    }
    
    // Default output folder
    if (outputFolder.isEmpty()) {
        outputFolder = folderPath;
    } else {
        if (!QDir(outputFolder).exists()) {
            QDir().mkpath(outputFolder);
        }
    }

    if (maskPath.isEmpty() || !QFile::exists(maskPath)) {
        QMessageBox::warning(this, "错误", "无效的遮罩文件");
        return;
    }

    m_startBtn->setEnabled(false);
    m_progressBar->setValue(0);
    m_logArea->clear();
    log("正在处理...");

    // Run in background
    (void)QtConcurrent::run([=]() {
        QDir dir(folderPath);
        QDir outDir(outputFolder); 
        
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tga" << "*.blp";
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
        int total = fileList.size();
        int current = 0;

        ImageProcessor processor;
        if (!processor.loadMask(maskPath, invertTga)) {
             QMetaObject::invokeMethod(this, [=]() { 
                log("错误: 无法加载遮罩文件。");
                m_startBtn->setEnabled(true);
             });
             return;
        }

        for (const QFileInfo& info : fileList) {
            // Updated logic to use outDir
             // Skip mask file itself if it's in the same folder AND we are writing deep into same folder
            if (info.absoluteFilePath() == QFileInfo(maskPath).absoluteFilePath()) continue;

            QString relPath = info.fileName();
            QString baseName = info.completeBaseName();
            QString ext = info.suffix().toLower();
            QString newExt = ext;

            if (formatIndex == 0) newExt = "png";
            else if (formatIndex == 1) newExt = "tga";
            else if (formatIndex == 2) newExt = "jpg";
            else if (formatIndex == 3) newExt = "blp";
            
            QString outputName = prefix + baseName + suffix + "." + newExt;
            QString outputPath = outDir.absoluteFilePath(outputName);

            QString error = processor.processImage(info.absoluteFilePath(), outputPath, blendMode, invertTga);
            bool success = error.isEmpty();
            
            QString logMsg = success ? QString("已处理: %1 -> %2").arg(relPath, outputName) 
                                     : QString("失败: %1 (%2)").arg(relPath, error);
            current++;
            QMetaObject::invokeMethod(this, [=]() {
                log(logMsg);
                m_progressBar->setValue((current * 100) / total);
            });
        }
        
        QMetaObject::invokeMethod(this, [=]() {
            log("处理完成。");
            m_startBtn->setEnabled(true);
        });
    });
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl& url : urlList) {
            QString path = url.toLocalFile();
            QFileInfo fi(path);
            
            if (fi.isDir()) {
                m_inputFolderEdit->setText(path);
                log("已设定输入文件夹: " + path);
                updateSampleImage(); // Trigger preview
            } else if (fi.isFile()) {
                QString ext = fi.suffix().toLower();
                if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "blp") {
                    m_maskFileEdit->setText(path);
                    log("已设定遮罩文件: " + path);
                    // TextChanged might catch this or we might need manual trigger if setText doesn't
                    // Programmatic setText DOES NOT trigger textChanged usually? Actually check doc.
                    // QLineEdit::setText usually does specific things.
                    // Let's force update
                }
            }
        }
        event->acceptProposedAction();
    }
}
