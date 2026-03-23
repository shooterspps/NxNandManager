#include "progress.h"
#include "ui_progress.h"
#include "mainwindow.h"
#include <QTextStream>

Progress::Progress(QWidget *parent, NxStorage *workingStorage) :
    QDialog(parent),
    ui(new Ui::Progress),
    m_parent(parent), m_workingStorage(workingStorage)
{
    ui->setupUi(this);
    this->setVisible(false);
    this->setWindowFlag(Qt::Popup);
    //this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    this->setWindowTitle("进度");

    ui->progressBar1->setFormat("");
    ui->progressBar2->setFormat("");
    setProgressBarStyle(ui->progressBar1, "CFCFCF");
    setProgressBarStyle(ui->progressBar2, "CFCFCF");


    auto mainWin = reinterpret_cast<MainWindow*>(parent);
    TaskBarButton = mainWin->get_TaskBarButton();
    TaskBarProgress = TaskBarButton->progress();
    TaskBarProgress->setVisible(true);

    m_buf_time = std::chrono::system_clock::now();
    // Init timer
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer1000()));
    timer->start(1000);
}

Progress::~Progress()
{
    if (TaskBarProgress) {
        TaskBarProgress->setValue(0);
        TaskBarProgress->setVisible(false);
    }
    if (console)
        delete console;
    if (console_progress_line)
        delete console_progress_line;
    delete ui;
}

void Progress::timer1000()
{
    auto time = std::chrono::system_clock::now();
    if(m_isRunning || b_done)
    {
        QString label;
        //elapsed time

        std::chrono::duration<double> elapsed_seconds = time - m_begin_time;
        label.append("已用时间: " + QString(GetReadableElapsedTime(elapsed_seconds).c_str()));

        //Remaining time
        if(!b_done && !b_simpleProgress && m_remaining_time >= time)
        {
            std::chrono::duration<double> remaining_seconds = m_remaining_time - time;
            label.append(" / 剩余时间: " + QString(GetReadableElapsedTime(remaining_seconds).c_str()));
        }
        ui->elapsed_time_label->setText(label);
        b_done = false;
    }

    // Transfer rate
    if(m_isRunning && !b_simpleProgress && m_cur_pi.mode != MD5_HASH)
    {
        std::chrono::duration<double> elapsed_seconds = time - m_buf_time;
        m_bytesProcessedPerSecond = (m_cur_pi.bytesCount - m_bytesCountBuffer) / elapsed_seconds.count();
        m_bytesCountBuffer = m_cur_pi.bytesCount;
        m_buf_time = time;

        if (m_bytesProcessedPerSecond < 0x200)
            m_bytesProcessedPerSecond = 0;

        if (m_l_bytesProcessedPerSecond.size() == 6)
            m_l_bytesProcessedPerSecond.erase(m_l_bytesProcessedPerSecond.begin());
        m_l_bytesProcessedPerSecond.push_back(m_bytesProcessedPerSecond);

        m_bytesProcessedPerSecond = 0;
        for (auto& bytes : m_l_bytesProcessedPerSecond)
            m_bytesProcessedPerSecond += bytes;

        m_bytesProcessedPerSecond = m_bytesProcessedPerSecond / m_l_bytesProcessedPerSecond.size();

        ui->transfertRateLbl->setText("传输速度: " + (m_bytesProcessedPerSecond ? QString::fromStdString(GetReadableSize(m_bytesProcessedPerSecond)) : QString("0b")) + "/s");

    }
    else
    {
        ui->transfertRateLbl->setText("");
    }
}

void Progress::updateProgress(const ProgressInfo pi)
{
    m_isRunning = true;
    if (!this->isVisible())
    {
        this->setVisible(true);
        this->setFocus();
    }
    auto time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = time - pi.begin_time;
    QString label;

    QProgressBar* progressBar;
    if(pi.isSubProgressInfo)
        progressBar = ui->progressBar2;
    else
    {
        if (is_in(pi.mode, {MD5_HASH}))
        {
            if (pi.bytesCount == pi.bytesTotal)
            {
                progressBar = ui->progressBar1;
                setProgressBarStyle(progressBar, "0FB3FF");
            }
            else progressBar = ui->progressBar2;
        }
        else
            progressBar = ui->progressBar1;
    }

    if (!pi.bytesCount)
    {
        progressBar->setValue(0);
        QString color = nullptr;
        if(pi.mode == MD5_HASH) color = "0FB3FF";
        else if (pi.mode == ZIP) color = "FF6A00";
        setProgressBarStyle(progressBar, color);
        progressBar->setRange(0, 100);

        // Initialize Main Progress Bar
        if(!pi.isSubProgressInfo)
        {
            b_simpleProgress = pi.percent == -1;
            TaskBarProgress->setValue(0);
            // First init
            if (not_in(pi.mode, {MD5_HASH, ZIP}))
            {
                m_begin_time = pi.begin_time;
            }
            // Simple progress, init sub progress
            if (b_simpleProgress) {
                ui->progressBar2->setRange(0, 0);
                setProgressBarStyle(ui->progressBar2, nullptr);
            }
        }
    }

    if (pi.bytesCount == pi.bytesTotal)
    {
        if (b_simpleProgress) {
            ui->progressBar2->setRange(0, 100);
            ui->progressBar2->setValue(100);
        }
        progressBar->setValue(100);
        if (!(pi.isSubProgressInfo && b_simpleProgress))
        {
            // Default label
            label.append(pi.storage_name);
            if (pi.mode == MD5_HASH) label.append(" 备份 & 校验完成");
            else if (pi.mode == RESTORE) label.append(" 还原完成");
            else if (pi.mode == RESIZE) label.append(" 调整完成");
            else if (pi.mode == CREATE) label.append(" 创建完成");
            else if (pi.mode == ZIP) label.append(pi.isSubProgressInfo ? " zip 完成" : " 压缩完成");
            else if (pi.mode == FORMAT) label.append(" 格式化完成");
            else if (pi.mode == EXTRACT) label.append(" 提取完成");
            else if (pi.mode == DECRYPT) label.append(" 解密完成");
            else label.append(" 备份完成");
            if (!b_simpleProgress)
                label.append(" (").append(GetReadableSize(pi.bytesTotal).c_str()).append(")");
        }
        if (!pi.isSubProgressInfo)
        {
            TaskBarProgress->setValue(100);
            setProgressBarStyle(ui->progressBar2, "CFCFCF");
            ui->progressBar2->setValue(100);
            ui->progressBar2->setFormat("");
        }

        if (console_progress_line)
            console_progress_line->setText("");
        b_done = true;
    }
    else
    {                
        int percent = pi.bytesCount * 100 / pi.bytesTotal;
        if (pi.bytesTotal > 1)
            progressBar->setValue(percent);

        if (!(pi.isSubProgressInfo && b_simpleProgress))
        {
            // Default label
            if (pi.mode == MD5_HASH) label.append("计算哈希值 ");
            else if (pi.mode == RESTORE) label.append("还原为 ");
            else if (pi.mode == RESIZE) label.append("调整大小");
            else if (pi.mode == CREATE) label.append("正在创建 ");
            else if (pi.mode == FORMAT) label.append("正在格式化 ");
            else if (pi.mode == ZIP) label.append(pi.isSubProgressInfo ? "正在压缩 " : "正在创建压缩 ");
            else if (pi.mode == EXTRACT) label.append("正在提取 ");
            else if (pi.mode == DECRYPT) label.append("正在解密 ");
            else label.append("正在复制 ");
            label.append(pi.storage_name);
            if (!b_simpleProgress) {
                label.append("... ").append(GetReadableSize(pi.bytesCount).c_str());
                label.append(" /").append(GetReadableSize(pi.bytesTotal).c_str());
                label.append(" (").append(QString::number(percent)).append("%)");
            }
            else if (pi.bytesTotal > 1)
                label.append(QString(" (%1/%2)").arg(pi.bytesCount+1).arg(pi.bytesTotal));
        }
        else if(pi.bytesTotal > 1) // Simple mode sub info
        {
            progressBar->setRange(0, 100);
            progressBar->setValue(percent);
            label = QString("%1/%2 %3%").arg(GetReadableSize(pi.bytesCount).c_str())
                                        .arg(GetReadableSize(pi.bytesTotal).c_str())
                                        .arg(percent);

        }
        if(!pi.isSubProgressInfo) {
            std::chrono::duration<double> remaining_seconds = (elapsed_seconds / pi.bytesCount) * (pi.bytesTotal - pi.bytesCount);
            m_remaining_time = time + remaining_seconds;
            TaskBarProgress->setValue(percent);
        }
        b_done = false;
    }
    progressBar->setFormat(label);
    progressBar->setTextVisible(true);

    // Save current ProgressInfo
    if(!pi.isSubProgressInfo)
        m_cur_pi = pi;
}

void Progress::setProgressBarStyle(QProgressBar* progressBar, QString color)
{
    QString st;
    if (nullptr == color || color == "rainbow")
        st.append(
        "QProgressBar::chunk {"
        "background: qlineargradient(x1: 0, y1: 0.5, x2: 1, y2: 0.5, stop: 0 #d3abd7, stop: 0.2 #acabfe, stop: 0.4 #b4d9ab, stop: 0.6 #ffffab, stop: 0.8 #ffdcab, stop: 1 #ffadab);"
        "} ");
    else st.append(
        "QProgressBar::chunk {"
        "background-color: #" + color + ";"
                                        "}");

    st.append("QProgressBar {"
              "border: 1px solid grey;"
              "border-radius: 2px;"
              "text-align: center;"
              "background: #eeeeee;"
              "}");
    progressBar->setStyleSheet(st);
}

void Progress::reject()
{
    if (m_isRunning)
    {
        if(QMessageBox::question(this, "警告", "确认中止, 取消当前进行的任务.\n任务正在进行中. 是否确认退出 ?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            if (nullptr != m_workingStorage)
            {
                m_workingStorage->stopWork = true;
                return;
            }
            TaskBarProgress->setVisible(false);
            QDialog::reject();
        }
        return;
    }
    TaskBarProgress->setVisible(false);
    QDialog::done(result());
}

void Progress::error(int err, QString label)
{
    this->setResult(err);
    m_isRunning = false;
    if(label != nullptr)
    {
        QMessageBox::critical(nullptr,"错误", label);
        this->reject();
        return;
    }

    for (int i=0; i < (int)array_countof(ErrorLabelArr); i++)
    {
        if(ErrorLabelArr[i].error == err) {
            QMessageBox::critical(nullptr,"错误", QString(ErrorLabelArr[i].label));
            this->reject();
            return;
        }
    }
    QMessageBox::critical(nullptr,"错误","错误 " + QString::number(err));
}
void Progress::on_WorkFinished()
{
    m_isRunning = false;
    setResult(QDialog::Accepted);
}

void Progress::on_pushButton_clicked()
{
    reject();
}

void Progress::consoleWrite(const QString &str)
{
    if (!console)
    {
        console = new QPlainTextEdit(this);
        console->setMinimumHeight(150);
        ui->consoleLayout->addWidget(console);
    }
    console->moveCursor(QTextCursor::End);
    console->textCursor().insertText(str);
    console->moveCursor(QTextCursor::End);

}
