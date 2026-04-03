#include "MainWindow.h"
#include "ImageProcessor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QApplication>
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
#include <QScrollArea>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QFormLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentDropTarget(nullptr)
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
        "QLineEdit[dropTarget=\"true\"] { border: 2px solid %5; background-color: #ECF5FF; }"
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

    QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    rootLayout->addWidget(scrollArea);

    QWidget* contentWidget = new QWidget();
    scrollArea->setWidget(contentWidget);

    // Main Layout
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
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

    // Base File Row
    QLabel* baseLabel = new QLabel("底图文件:");
    baseLabel->setFixedWidth(90);
    m_baseFileEdit = new QLineEdit();
    m_baseFileEdit->setPlaceholderText("拖入 base.png 图片...");
    connect(m_baseFileEdit, &QLineEdit::textChanged, this, &MainWindow::updatePreview);
    QPushButton* browseBaseBtn = new QPushButton("浏览");
    connect(browseBaseBtn, &QPushButton::clicked, this, &MainWindow::browseBaseFile);
    sourceLayout->addWidget(baseLabel, 4, 0);
    sourceLayout->addWidget(m_baseFileEdit, 4, 1);
    sourceLayout->addWidget(browseBaseBtn, 4, 2);

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
    p1->addWidget(new QLabel("底图 (Base)"), 0, Qt::AlignCenter);
    m_previewOriginal = createPreviewLabel("等待载入...");
    p1->addWidget(m_previewOriginal, 0, Qt::AlignCenter);
    
    QVBoxLayout* p2 = new QVBoxLayout();
    p2->addWidget(new QLabel("原图 (Sample)"), 0, Qt::AlignCenter);
    m_previewBase = createPreviewLabel("等待载入...");
    p2->addWidget(m_previewBase, 0, Qt::AlignCenter);

    QVBoxLayout* p3 = new QVBoxLayout();
    p3->addWidget(new QLabel("遮罩 (Mask)"), 0, Qt::AlignCenter);
    m_previewMask = createPreviewLabel("等待载入...");
    p3->addWidget(m_previewMask, 0, Qt::AlignCenter);

    QVBoxLayout* p4 = new QVBoxLayout();
    p4->addWidget(new QLabel("结果 (Result)"), 0, Qt::AlignCenter);
    m_previewResult = createPreviewLabel("等待处理...");
    m_previewResult->setFixedSize(256, 256);
    m_previewResult->setCursor(Qt::PointingHandCursor);
    m_previewResult->setToolTip("双击查看放大预览");
    m_previewResult->installEventFilter(this);
    p4->addWidget(m_previewResult, 0, Qt::AlignCenter);

    previewImagesLayout->addLayout(p1);
    previewImagesLayout->addWidget(new QLabel("+"), 0, Qt::AlignCenter);
    previewImagesLayout->addLayout(p2);
    previewImagesLayout->addWidget(new QLabel("+"), 0, Qt::AlignCenter);
    previewImagesLayout->addLayout(p3);
    previewImagesLayout->addWidget(new QLabel("="), 0, Qt::AlignCenter);
    previewImagesLayout->addLayout(p4);

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

    // Output Size Mode
    QHBoxLayout* outputSizeLayout = new QHBoxLayout();
    outputSizeLayout->addWidget(new QLabel("输出尺寸:"));
    m_outputSizeModeCombo = new QComboBox();
    m_outputSizeModeCombo->addItem("以遮罩尺寸输出");
    m_outputSizeModeCombo->addItem("以普通图尺寸输出");
    m_outputSizeModeCombo->addItem("以底图尺寸输出");
    m_outputSizeModeCombo->setCurrentIndex(1);
    connect(m_outputSizeModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    outputSizeLayout->addWidget(m_outputSizeModeCombo);
    optionsLayout->addLayout(outputSizeLayout);

    m_tileLayersCheckbox = new QCheckBox("快速平铺开关 (开=重复平铺，关=拉伸)");
    connect(m_tileLayersCheckbox, &QCheckBox::checkStateChanged, this, [this]() {
        m_originalTileModeCombo->setCurrentIndex(m_tileLayersCheckbox->isChecked() ? ImageProcessor::OriginalRepeat : ImageProcessor::OriginalStretch);
        updatePreview();
    });
    optionsLayout->addWidget(m_tileLayersCheckbox);

    QFormLayout* tileForm = new QFormLayout();
    tileForm->setSpacing(8);

    m_originalTileModeCombo = new QComboBox();
    m_originalTileModeCombo->addItem("拉伸匹配");
    m_originalTileModeCombo->addItem("重复平铺");
    m_originalTileModeCombo->addItem("镜像平铺");
    m_originalTileModeCombo->addItem("单次定位");
    m_originalTileModeCombo->addItem("边缘拉伸填充(Clamp)");
    m_originalTileModeCombo->setCurrentIndex(ImageProcessor::OriginalStretch);
    connect(m_originalTileModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        const int mode = m_originalTileModeCombo->currentIndex();
        if (mode == ImageProcessor::OriginalStretch) {
            m_tileLayersCheckbox->setChecked(false);
        } else if (mode == ImageProcessor::OriginalRepeat) {
            m_tileLayersCheckbox->setChecked(true);
        } else {
            updatePreview();
        }
    });
    tileForm->addRow("原图模式:", m_originalTileModeCombo);

    m_originalScaleModeCombo = new QComboBox();
    m_originalScaleModeCombo->addItem("百分比缩放");
    m_originalScaleModeCombo->addItem("自动适配单块(AutoFit)");
    m_originalScaleModeCombo->addItem("1:1 原始像素");
    connect(m_originalScaleModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("缩放模式:", m_originalScaleModeCombo);

    m_originalScalePercentSpin = new QDoubleSpinBox();
    m_originalScalePercentSpin->setRange(1.0, 1000.0);
    m_originalScalePercentSpin->setSingleStep(5.0);
    m_originalScalePercentSpin->setDecimals(1);
    m_originalScalePercentSpin->setValue(100.0);
    m_originalScalePercentSpin->setSuffix(" %");
    connect(m_originalScalePercentSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("缩放比例:", m_originalScalePercentSpin);

    m_originalAnchorCombo = new QComboBox();
    m_originalAnchorCombo->addItem("居中");
    m_originalAnchorCombo->addItem("左上");
    m_originalAnchorCombo->addItem("右上");
    m_originalAnchorCombo->addItem("左下");
    m_originalAnchorCombo->addItem("右下");
    connect(m_originalAnchorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("锚点:", m_originalAnchorCombo);

    m_originalOffsetXSpin = new QSpinBox();
    m_originalOffsetXSpin->setRange(-4096, 4096);
    m_originalOffsetXSpin->setValue(0);
    connect(m_originalOffsetXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("偏移 X:", m_originalOffsetXSpin);

    m_originalOffsetYSpin = new QSpinBox();
    m_originalOffsetYSpin->setRange(-4096, 4096);
    m_originalOffsetYSpin->setValue(0);
    connect(m_originalOffsetYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("偏移 Y:", m_originalOffsetYSpin);

    m_originalDirectionCombo = new QComboBox();
    m_originalDirectionCombo->addItem("X + Y");
    m_originalDirectionCombo->addItem("仅 X");
    m_originalDirectionCombo->addItem("仅 Y");
    connect(m_originalDirectionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("平铺方向:", m_originalDirectionCombo);

    m_originalEdgeCombo = new QComboBox();
    m_originalEdgeCombo->addItem("裁剪");
    m_originalEdgeCombo->addItem("透明边");
    m_originalEdgeCombo->addItem("边缘羽化");
    connect(m_originalEdgeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("边缘处理:", m_originalEdgeCombo);

    m_originalQualityCombo = new QComboBox();
    m_originalQualityCombo->addItem("平滑质量");
    m_originalQualityCombo->addItem("快速预览");
    connect(m_originalQualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updatePreview);
    tileForm->addRow("缩放质量:", m_originalQualityCombo);

    optionsLayout->addLayout(tileForm);

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
    resize(980, 760);
    setMinimumSize(760, 560);
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

void MainWindow::setDropTargetHighlight(QLineEdit* target)
{
    if (m_currentDropTarget == target) {
        return;
    }

    if (m_currentDropTarget) {
        m_currentDropTarget->setProperty("dropTarget", false);
        m_currentDropTarget->style()->unpolish(m_currentDropTarget);
        m_currentDropTarget->style()->polish(m_currentDropTarget);
        m_currentDropTarget->update();
    }

    m_currentDropTarget = target;

    if (m_currentDropTarget) {
        m_currentDropTarget->setProperty("dropTarget", true);
        m_currentDropTarget->style()->unpolish(m_currentDropTarget);
        m_currentDropTarget->style()->polish(m_currentDropTarget);
        m_currentDropTarget->update();
    }
}

void MainWindow::clearDropTargetHighlight()
{
    setDropTargetHighlight(nullptr);
}

void MainWindow::browseInputFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择输入文件夹");
    if (!dir.isEmpty()) {
        m_inputFolderEdit->setText(dir);
        updateSampleImage();
    }
}

void MainWindow::browseMaskFile()
{
    QString file = QFileDialog::getOpenFileName(this, "选择遮罩文件", QString(), "Images (*.png *.jpg *.bmp *.tga *.blp)");
    if (!file.isEmpty()) {
        m_maskFileEdit->setText(file);
    }
}

void MainWindow::browseBaseFile()
{
    QString file = QFileDialog::getOpenFileName(this, "选择底图文件", QString(), "Images (*.png *.jpg *.bmp *.tga *.blp)");
    if (!file.isEmpty()) {
        m_baseFileEdit->setText(file);
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
        m_lastPreviewSheet = QImage();
        m_previewOriginal->setText("无样本");
        m_previewBase->setText("无底图");
        m_previewMask->setText("无遮罩");
        m_previewResult->setText("请选择\n输入文件夹");
        return;
    }
    
    QString maskPath = m_maskFileEdit->text();
    QString basePath = m_baseFileEdit->text();
    if (maskPath.isEmpty() || !QFile::exists(maskPath)) {
        m_lastPreviewSheet = QImage();
        m_previewBase->setText("等待底图");
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

    if (basePath.isEmpty() || !QFile::exists(basePath)) {
        m_lastPreviewSheet = QImage();
        m_previewBase->setText("无底图");
        m_previewResult->setText("请选择\n底图文件");
        return;
    }

    int blendMode = m_blendModeCombo->currentIndex();
    bool invertTga = m_tgaAlphaInvertCheckbox->isChecked();
    int outputSizeMode = m_outputSizeModeCombo->currentIndex();
    int originalTileMode = m_originalTileModeCombo->currentIndex();
    int originalScaleMode = m_originalScaleModeCombo->currentIndex();
    double originalScalePercent = m_originalScalePercentSpin->value();
    int originalAnchorMode = m_originalAnchorCombo->currentIndex();
    int originalOffsetX = m_originalOffsetXSpin->value();
    int originalOffsetY = m_originalOffsetYSpin->value();
    int originalDirectionMode = m_originalDirectionCombo->currentIndex();
    int originalEdgeMode = m_originalEdgeCombo->currentIndex();
    int originalQualityMode = m_originalQualityCombo->currentIndex();
    const QString samplePath = m_sampleImagePath;

    // Run preview generation in background to avoid freezing UI
    (void)QtConcurrent::run([=]() {
        ImageProcessor processor;
        if (!processor.loadMask(maskPath, invertTga)) {
             QMetaObject::invokeMethod(this, [=]() { m_previewMask->setText("加载失败"); });
             return;
        }

        if (!processor.loadBase(basePath, invertTga)) {
            QMetaObject::invokeMethod(this, [=]() {
                m_previewOriginal->setText("加载失败");
                m_previewResult->setText("预览失败\n底图加载失败");
                m_lastPreviewSheet = QImage();
            });
            return;
        }

        QImage resultImg;
        QString previewError = processor.processImagePreview(
            samplePath,
            blendMode,
            invertTga,
            outputSizeMode,
            originalTileMode,
            originalScaleMode,
            originalScalePercent,
            originalAnchorMode,
            originalOffsetX,
            originalOffsetY,
            originalDirectionMode,
            originalEdgeMode,
            originalQualityMode,
            resultImg
        );

        QImage inputImg = ImageProcessor::loadTGA(samplePath, invertTga);
        if (inputImg.isNull()) {
            inputImg.load(samplePath);
        }

        QImage maskImg = ImageProcessor::loadTGA(maskPath, invertTga);
        if (maskImg.isNull()) {
            maskImg.load(maskPath);
        }

        QImage baseImg = ImageProcessor::loadTGA(basePath, invertTga);
        if (baseImg.isNull()) {
            baseImg.load(basePath);
        }

        // Update UI
        QMetaObject::invokeMethod(this, [=]() {
            if (!baseImg.isNull()) {
                m_previewOriginal->setPixmap(QPixmap::fromImage(baseImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                m_previewOriginal->setText("加载失败");
            }

            if (!inputImg.isNull()) {
                m_previewBase->setPixmap(QPixmap::fromImage(inputImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                m_previewBase->setText("加载失败");
            }

            if (!maskImg.isNull()) {
                m_previewMask->setPixmap(QPixmap::fromImage(maskImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                m_previewMask->setText("加载失败");
            }

            if (previewError.isEmpty() && !resultImg.isNull()) {
                m_lastPreviewSheet = resultImg;
                m_previewResult->setPixmap(QPixmap::fromImage(resultImg.scaled(
                    m_previewResult->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                )));
            } else {
                m_lastPreviewSheet = QImage();
                if (!previewError.isEmpty()) {
                    m_previewResult->setText("预览失败\n" + previewError);
                } else {
                    m_previewResult->setText("预览失败");
                }
            }
        });

    });
}

void MainWindow::startProcessing(void)
{
    QString folderPath = m_inputFolderEdit->text();
    QString outputFolder = m_outputFolderEdit->text(); // New
    QString maskPath = m_maskFileEdit->text();
    QString basePath = m_baseFileEdit->text();
    QString prefix = m_prefixEdit->text();
    QString suffix = m_suffixEdit->text();
    int formatIndex = m_outputFormatCombo->currentIndex();
    int blendMode = m_blendModeCombo->currentIndex();
    bool invertTga = m_tgaAlphaInvertCheckbox->isChecked();
    int outputSizeMode = m_outputSizeModeCombo->currentIndex();
    int originalTileMode = m_originalTileModeCombo->currentIndex();
    int originalScaleMode = m_originalScaleModeCombo->currentIndex();
    double originalScalePercent = m_originalScalePercentSpin->value();
    int originalAnchorMode = m_originalAnchorCombo->currentIndex();
    int originalOffsetX = m_originalOffsetXSpin->value();
    int originalOffsetY = m_originalOffsetYSpin->value();
    int originalDirectionMode = m_originalDirectionCombo->currentIndex();
    int originalEdgeMode = m_originalEdgeCombo->currentIndex();
    int originalQualityMode = m_originalQualityCombo->currentIndex();

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

    if (basePath.isEmpty() || !QFile::exists(basePath)) {
        QMessageBox::warning(this, "错误", "无效的底图文件");
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

        if (!processor.loadBase(basePath, invertTga)) {
            QMetaObject::invokeMethod(this, [=]() {
                log("错误: 无法加载底图文件。");
                m_startBtn->setEnabled(true);
            });
            return;
        }

        for (const QFileInfo& info : fileList) {
            // Updated logic to use outDir
             // Skip mask file itself if it's in the same folder AND we are writing deep into same folder
            if (info.absoluteFilePath() == QFileInfo(maskPath).absoluteFilePath()) continue;
            if (info.absoluteFilePath() == QFileInfo(basePath).absoluteFilePath()) continue;

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

            QString error = processor.processImage(info.absoluteFilePath(), outputPath, blendMode, invertTga,
                                                  outputSizeMode, originalTileMode, originalScaleMode,
                                                  originalScalePercent, originalAnchorMode, originalOffsetX,
                                                  originalOffsetY, originalDirectionMode, originalEdgeMode,
                                                  originalQualityMode);
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

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        clearDropTargetHighlight();
        event->ignore();
        return;
    }

    QWidget* hoverWidget = QApplication::widgetAt(mapToGlobal(event->position().toPoint()));
    QLineEdit* target = nullptr;

    auto hitLineEdit = [hoverWidget](QLineEdit* edit) {
        return hoverWidget && (hoverWidget == edit || edit->isAncestorOf(hoverWidget));
    };

    if (hitLineEdit(m_inputFolderEdit)) {
        target = m_inputFolderEdit;
    } else if (hitLineEdit(m_outputFolderEdit)) {
        target = m_outputFolderEdit;
    } else if (hitLineEdit(m_maskFileEdit)) {
        target = m_maskFileEdit;
    } else if (hitLineEdit(m_baseFileEdit)) {
        target = m_baseFileEdit;
    }

    setDropTargetHighlight(target);
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event);
    clearDropTargetHighlight();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        return;
    }

    auto isSupportedImage = [](const QFileInfo& fi) {
        const QString ext = fi.suffix().toLower();
        return fi.isFile() && (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "blp");
    };

    QWidget* dropTarget = QApplication::widgetAt(mapToGlobal(event->position().toPoint()));
    auto droppedOnLineEdit = [dropTarget](QLineEdit* edit) {
        return dropTarget && (dropTarget == edit || edit->isAncestorOf(dropTarget));
    };

    bool droppedOnMask = droppedOnLineEdit(m_maskFileEdit);
    bool droppedOnBase = droppedOnLineEdit(m_baseFileEdit);
    bool droppedOnInput = droppedOnLineEdit(m_inputFolderEdit);
    bool droppedOnOutput = droppedOnLineEdit(m_outputFolderEdit);

    QList<QUrl> urlList = mimeData->urls();
    QStringList imagePaths;
    bool inputFolderChanged = false;

    for (const QUrl& url : urlList) {
        const QString path = url.toLocalFile();
        QFileInfo fi(path);
        if (fi.isDir()) {
            if (droppedOnOutput) {
                m_outputFolderEdit->setText(path);
                log("已设定输出文件夹: " + path);
            } else if (droppedOnInput) {
                m_inputFolderEdit->setText(path);
                log("已设定输入文件夹: " + path);
                inputFolderChanged = true;
            } else if (m_inputFolderEdit->text().isEmpty()) {
                m_inputFolderEdit->setText(path);
                log("已设定输入文件夹: " + path);
                inputFolderChanged = true;
            } else if (m_outputFolderEdit->text().isEmpty()) {
                m_outputFolderEdit->setText(path);
                log("已设定输出文件夹: " + path);
            } else {
                m_inputFolderEdit->setText(path);
                log("已设定输入文件夹: " + path);
                inputFolderChanged = true;
            }
            continue;
        }

        if (isSupportedImage(fi)) {
            imagePaths << fi.absoluteFilePath();
        }
    }

    if (!imagePaths.isEmpty()) {
        if (imagePaths.size() == 1 && droppedOnMask) {
            m_maskFileEdit->setText(imagePaths.first());
            log("已设定遮罩文件: " + imagePaths.first());
        } else if (imagePaths.size() == 1 && droppedOnBase) {
            m_baseFileEdit->setText(imagePaths.first());
            log("已设定底图文件: " + imagePaths.first());
        } else {
            for (const QString& imagePath : imagePaths) {
                const QString name = QFileInfo(imagePath).fileName().toLower();

                if (name.contains("mask") || name.contains("alpha")) {
                    m_maskFileEdit->setText(imagePath);
                    log("已设定遮罩文件: " + imagePath);
                    continue;
                }
                if (name.contains("base") || name.contains("background") || name.contains("底图")) {
                    m_baseFileEdit->setText(imagePath);
                    log("已设定底图文件: " + imagePath);
                    continue;
                }

                if (m_baseFileEdit->text().isEmpty() || droppedOnBase) {
                    m_baseFileEdit->setText(imagePath);
                    log("已设定底图文件: " + imagePath);
                } else {
                    m_maskFileEdit->setText(imagePath);
                    log("已设定遮罩文件: " + imagePath);
                }
            }
        }
    }

    if (inputFolderChanged || droppedOnInput) {
        updateSampleImage();
    }
    updatePreview();
    clearDropTargetHighlight();
    event->acceptProposedAction();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_previewResult && event->type() == QEvent::MouseButtonDblClick) {
        if (m_lastPreviewSheet.isNull()) {
            return true;
        }

        QDialog dlg(this);
        dlg.setWindowTitle("结果放大预览");
        dlg.resize(980, 760);

        QVBoxLayout* layout = new QVBoxLayout(&dlg);

        const QImage img = m_lastPreviewSheet;
        QString formatName = "Unknown";
        switch (img.format()) {
            case QImage::Format_ARGB32: formatName = "ARGB32"; break;
            case QImage::Format_ARGB32_Premultiplied: formatName = "ARGB32_Premultiplied"; break;
            case QImage::Format_RGB32: formatName = "RGB32"; break;
            case QImage::Format_RGB888: formatName = "RGB888"; break;
            case QImage::Format_RGBA8888: formatName = "RGBA8888"; break;
            default: break;
        }

        const double memoryKB = static_cast<double>(img.sizeInBytes()) / 1024.0;
        QLabel* infoLabel = new QLabel(QString(
            "尺寸: %1 x %2    格式: %3    位深: %4-bit    Alpha: %5    内存占用: %6 KB"
        )
            .arg(img.width())
            .arg(img.height())
            .arg(formatName)
            .arg(img.depth())
            .arg(img.hasAlphaChannel() ? "是" : "否")
            .arg(QString::number(memoryKB, 'f', 1)));
        infoLabel->setStyleSheet("padding: 6px 8px; background: #F5F7FA; border: 1px solid #DFE4EA; border-radius: 4px; color: #606266;");
        infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(infoLabel);

        QScrollArea* area = new QScrollArea(&dlg);
        area->setWidgetResizable(false);
        area->setAlignment(Qt::AlignCenter);

        QLabel* imageLabel = new QLabel();
        imageLabel->setPixmap(QPixmap::fromImage(m_lastPreviewSheet));
        imageLabel->setAlignment(Qt::AlignCenter);
        area->setWidget(imageLabel);

        layout->addWidget(area);

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        layout->addWidget(buttons);

        dlg.exec();
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}
