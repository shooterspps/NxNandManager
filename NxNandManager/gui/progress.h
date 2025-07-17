﻿#ifndef PROGRESS_H
#define PROGRESS_H

#include <QDialog>
#include <QtWinExtras>
#include <QMessageBox>
#include <QProgressBar>
#include <QWinTaskbarProgress>
#include "../NxStorage.h"
#include <QPlainTextEdit>

namespace Ui {
class Progress;
}

class Progress : public QDialog
{
    Q_OBJECT

public:
    explicit Progress(QWidget *parent = nullptr, NxStorage *workingStorage = nullptr);
    ~Progress();

signals:


public slots:
    void updateProgress(const ProgressInfo pi);
    void error(int err, QString label);
    void on_WorkFinished();
    void timer1000();
    void reject() override;
    void consoleWrite(const QString &str);

private slots:
    void on_pushButton_clicked();

protected:
    //void showEvent(QShowEvent *e) override;
private:
    // Pointers
    Ui::Progress *ui;
    QWidget* m_parent = nullptr;
    NxStorage *m_workingStorage;
    QWinTaskbarButton *TaskBarButton = nullptr;
    QWinTaskbarProgress *TaskBarProgress = nullptr;
    QPlainTextEdit* console = nullptr;
    QLabel* console_progress_line = nullptr;
    bool bTaskBarSet = false;
    // Member variables
    bool m_workerSet = false;
    bool m_isRunning = false;
    timepoint_t m_begin_time;
    timepoint_t m_remaining_time;
    ProgressInfo m_cur_pi;
    u64 m_bytesCountBuffer = 0;
    timepoint_t m_buf_time;
    u64 m_bytesProcessedPerSecond = 0;
    std::vector<u64> m_l_bytesProcessedPerSecond;
    bool b_done = false;
    bool b_simpleProgress;
    // Methods
    void setProgressBarStyle(QProgressBar* progressBar, QString color);
};

#endif // PROGRESS_H
