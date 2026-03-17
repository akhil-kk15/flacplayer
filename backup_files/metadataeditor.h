#ifndef METADATAEDITOR_H
#define METADATAEDITOR_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include "audiomanager.h"

class MetadataEditor : public QDialog
{
    Q_OBJECT

public:
    explicit MetadataEditor(const QString &filePath, QWidget *parent = nullptr);
    ~MetadataEditor();
    
    AudioMetadata getMetadata() const;

private slots:
    void onSave();
    void onCancel();

private:
    void setupUI();
    void loadCurrentMetadata();
    bool saveMetadata();
    
private:
    QString m_filePath;
    AudioMetadata m_metadata;
    
    // UI elements
    QLineEdit *m_titleEdit;
    QLineEdit *m_artistEdit;
    QLineEdit *m_albumEdit;
    QLineEdit *m_yearEdit;
    QLineEdit *m_genreEdit;
    QTextEdit *m_commentEdit;
    
    QLabel *m_statusLabel;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
};

#endif // METADATAEDITOR_H
