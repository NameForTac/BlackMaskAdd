#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void browseInputFolder();
    void browseOutputFolder();
    void browseMaskFile();
    void startProcessing();
    void updatePreview();

private:
    void setupUi();
    void log(const QString& message);
    void updateSampleImage();

    QLineEdit* m_inputFolderEdit;
    QLineEdit* m_outputFolderEdit;
    QLineEdit* m_maskFileEdit;
    QLineEdit* m_prefixEdit;
    QLineEdit* m_suffixEdit;
    QComboBox* m_outputFormatCombo;
    QComboBox* m_blendModeCombo;
    QCheckBox* m_tgaAlphaInvertCheckbox; 
    QPushButton* m_startBtn;
    QProgressBar* m_progressBar;
    QTextEdit* m_logArea;

    // Preview
    QString m_sampleImagePath;
    QLabel* m_previewOriginal;
    QLabel* m_previewMask;
    QLabel* m_previewResult;
};
