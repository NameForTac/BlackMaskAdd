#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void browseInputFolder();
    void browseOutputFolder();
    void browseMaskFile();
    void browseBaseFile();
    void startProcessing();
    void updatePreview();

private:
    void setupUi();
    void log(const QString& message);
    void updateSampleImage();
    void setDropTargetHighlight(QLineEdit* target);
    void clearDropTargetHighlight();

    QLineEdit* m_inputFolderEdit;
    QLineEdit* m_outputFolderEdit;
    QLineEdit* m_maskFileEdit;
    QLineEdit* m_baseFileEdit;
    QLineEdit* m_prefixEdit;
    QLineEdit* m_suffixEdit;
    QComboBox* m_outputFormatCombo;
    QComboBox* m_blendModeCombo;
    QComboBox* m_outputSizeModeCombo;
    QComboBox* m_originalTileModeCombo;
    QComboBox* m_originalScaleModeCombo;
    QDoubleSpinBox* m_originalScalePercentSpin;
    QComboBox* m_originalAnchorCombo;
    QSpinBox* m_originalOffsetXSpin;
    QSpinBox* m_originalOffsetYSpin;
    QComboBox* m_originalDirectionCombo;
    QComboBox* m_originalEdgeCombo;
    QComboBox* m_originalQualityCombo;
    QCheckBox* m_tgaAlphaInvertCheckbox; 
    QCheckBox* m_tileLayersCheckbox;
    QPushButton* m_startBtn;
    QProgressBar* m_progressBar;
    QTextEdit* m_logArea;

    // Preview
    QString m_sampleImagePath;
    QLabel* m_previewOriginal;
    QLabel* m_previewBase;
    QLabel* m_previewMask;
    QLabel* m_previewResult;
    QImage m_lastPreviewSheet;
    QLineEdit* m_currentDropTarget;
};
