#ifndef CONVERSIONDIALOG_H
#define CONVERSIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QThread>
#include "audioconverter.h"

class ConversionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConversionDialog(const QString &inputFile, QWidget *parent = nullptr);
    ~ConversionDialog();

private slots:
    void onBrowseClicked();
    void onConvertClicked();
    void onCancelClicked();
    void onProgressUpdated(int percentage);
    void onConversionFinished(bool success, const QString &message);

private:
    void setupUI();
    QString generateOutputPath(const QString &inputPath);

    QString m_inputFile;
    
    QLabel *m_inputLabel;
    QLineEdit *m_outputEdit;
    QPushButton *m_browseButton;
    QComboBox *m_bitrateCombo;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_convertButton;
    QPushButton *m_cancelButton;
    
    QThread *m_workerThread;
    AudioConverterWorker *m_worker;
    
    bool m_converting;
};

#endif // CONVERSIONDIALOG_H
