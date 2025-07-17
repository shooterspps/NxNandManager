﻿/*
 * Copyright (c) 2019 eliboa
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtConcurrent/QtConcurrent>
#include <QtWidgets>


MainWindow *mainWindowInstance = nullptr;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
    mainWindowInstance = this;
	ui->setupUi(this);

    dumpIcon = QIcon(":/images/save.png");
    restoreIcon = QIcon(":/images/restore.png");
    encIcon = QIcon(":/images/decrypt.png");
    decIcon = QIcon(":/images/encrypt.png");
    rcmIcon = QIcon(":/images/autorcm.png");
    incoIcon = QIcon(":/images/incognito.png");
    formtIcon = QIcon(":/images/format.png");
    mountIcon = QIcon(":/images/drive.png");
    unmountIcon = QIcon(":/images/unmount.png");
    explorerIcon = QIcon(":/images/explorer.png");

    TaskBarButton = new QWinTaskbarButton(this);

    //setFixedWidth(380);
    //setFixedHeight(490);
    //adjustSize();
    qRegisterMetaType<NTSTATUS>("NTSTATUS");
    qRegisterMetaType<QVector<int>>("QVector<int>");

    // Connect slots
    connect(ui->actionOpenFile, &QAction::triggered, this, &MainWindow::open);
    connect(ui->actionOpenDrive, &QAction::triggered, this, &MainWindow::openDrive);
    connect(ui->actionCloseFileDrive, &QAction::triggered, this, &MainWindow::closeInput);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::on_rawdump_button);
    connect(ui->actionDecryptSaveAs, &QAction::triggered, this, &MainWindow::on_rawdumpDec_button_clicked);
    connect(ui->actionEncryptSaveAs, &QAction::triggered, this, &MainWindow::on_rawdumpEnc_button_clicked);
    connect(ui->actionRestore, &QAction::triggered, this, &MainWindow::on_fullrestore_button_clicked);
    connect(ui->actionProperties, &QAction::triggered, this, &MainWindow::Properties);
    connect(ui->actionConfigureKeyset, &QAction::triggered, this, &MainWindow::openKeySet);
    connect(ui->actionIncognito, &QAction::triggered, this, &MainWindow::incognito);
    connect(ui->actionautoRCM, &QAction::triggered, this, &MainWindow::toggleAutoRCM);
    connect(ui->actionResize, &QAction::triggered, this, &MainWindow::openResizeDialog);
    connect(ui->actionDumpRawnandOnly, &QAction::triggered, this, &MainWindow::dumpRAWNAND);
    connect(ui->actionCreateEmunand, &QAction::triggered, this, &MainWindow::openEmunandDialog);
    connect(ui->actionSaveAsAdvanced, &QAction::triggered, this, &MainWindow::openDumpDialog);
    connect(ui->actionDebug_console, &QAction::triggered, this, &MainWindow::openDebugDialog);
    connect(ui->partQDumpBtn, SIGNAL(clicked()), this, SLOT(dumpPartition()));
    connect(ui->partADumpBtn, SIGNAL(clicked()), this, SLOT(dumpPartitionAdvanced()));
    connect(ui->partRestoreBtn, SIGNAL(clicked()), this, SLOT(restorePartition()));
    connect(this, SIGNAL(error_signal(int, QString)), this, SLOT(error(int, QString)));
    connect(this, SIGNAL(dokanDriver_install_signal()), this, SLOT(dokanDriver_install()));
    connect(ui->actionRestartDebug, &QAction::triggered, this, &MainWindow::restartDebug);


    if (!isdebug)
    {
        ui->menuDebug->setDisabled(true);
        ui->menuDebug->menuAction()->setVisible(false);

    }
    else
    {
        ui->actionRestartDebug->setDisabled(true);
    }
    ui->partQDumpBtn->setDisabled(true);
    ui->partADumpBtn->setDisabled(true);
    ui->partRestoreBtn->setDisabled(true);
    ui->partQDumpBtn->setVisible(false);
    ui->partADumpBtn->setVisible(false);
    ui->partRestoreBtn->setVisible(false);
    ui->partCustom1Btn->setVisible(false);
    ui->partCustom1Btn->setDisabled(true);

    input = nullptr;

    ui->analysingLbl->setVisible(false);
    ui->loadingBar->setVisible(false);

	// Init buttons
	ui->rawdump_button->setEnabled(false);
	ui->fullrestore_button->setEnabled(false);

    // Keyset bool
    bKeyset = false;
    QFile file("keys.dat");
    if (file.exists())
        bKeyset = true;

    bool sw = false;
    QString input_path;
    for (QString arg : QCoreApplication::arguments()) {
        if (sw) {
            input_path.append(arg);
            break;
        }
        if (!arg.compare("-i")) sw = true;
    }

    if (isdebug)
        openDebugDialog();

    // Create title database
    m_titleDB = new NxTitleDB("all_titles.json",
                              "https://eliboa.com/switch/all_titles.php?export=json");

    // Create nca filename database
    m_ncaDB = new NxNcaDB("nca.json",
                          "https://eliboa.com/switch/nca.php?export=json",
                          0x54600); // exp. delay: 4 days

    if(input_path.count())
    {
        beforeInputSet();
        qApp->processEvents();
        if (workThread)
            delete workThread;
        workThread = new Worker(this, WorkerMode::new_storage, input_path);
        workThread->start();
    }

    ui->properties_table->setColumnCount(2);
    ui->properties_table->setColumnWidth(0, 80);
    ui->properties_table->setColumnWidth(1, 155);
}

MainWindow::~MainWindow()
{
    foreach (QAction *action, ui->partition_table->actions()) {
        ui->partition_table->removeAction(action);
        delete action;
    }
    delete TaskBarButton;
	delete ui;
    if (workThread)
        delete workThread;
    if (input)
        delete input;
    if (m_titleDB)
        delete m_titleDB;
    if (m_ncaDB)
        delete m_ncaDB;
}

void MainWindow::showEvent(QShowEvent *e)
{
	if(!bTaskBarSet)
	{
		TaskBarButton->setWindow(windowHandle());
		TaskBarProgress = TaskBarButton->progress();
        bTaskBarSet = true;
	}
	e->accept();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if(WorkInProgress)
	{
		if(QMessageBox::question(this, "警告", "任务进行中, 是否确定退出 ?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		{
			event->ignore();
			return;
		}
	}
	event->accept();
}
/*
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QSize oldSize = event->oldSize();
    QSize newSize = event->size();
    event->accept();
}
*/
void MainWindow::open()
{
    QString fileName = FileDialog(this, fdMode::open_file);
    if (fileName.isEmpty())
        return;

    if (!safe_closeInput())
        return;

    if (workThread)
        delete workThread;
    workThread = new Worker(this, WorkerMode::new_storage, fileName);
    workThread->start();

}
bool MainWindow::safe_closeInput()
{
    if (!input)
        return true;

    if(input->is_vfs_mounted() && QMessageBox::question(this, "警告",
                "至少有一个分区被挂载为虚拟磁盘.\n是否要卸载并关闭导入 ?\n ",
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return false;

    delete input;
    input = nullptr;
    beforeInputSet();
    ui->analysingLbl->setVisible(false);
    ui->loadingBar->setVisible(false);
    ui->partitionsGrp->setVisible(true);

    return true;
};

void MainWindow::closeInput()
{
    safe_closeInput();
};

void MainWindow::openDrive()
{
    if(openDriveDialog != nullptr)
    {
        if (openDriveDialog->isOpen)
        {
            openDriveDialog->setFocus();
            return;
        }
        else delete openDriveDialog;
    }
    openDriveDialog = new OpenDrive(this);
	openDriveDialog->show();    
	openDriveDialog->exec();
}

void MainWindow::Properties()
{
    if(PropertiesDialog != nullptr)
    {
        if (PropertiesDialog->isOpen)
        {
            PropertiesDialog->setFocus();
            return;
        }
        else delete PropertiesDialog;
    }
    PropertiesDialog = new class Properties(this->input);
    PropertiesDialog->setWindowTitle("属性");
    PropertiesDialog->show();
    PropertiesDialog->exec();
}

void MainWindow::openKeySet()
{
    keysetDialog = new KeySetDialog(this);
    keysetDialog->setWindowTitle("配置密钥");
    keysetDialog->show();
    keysetDialog->exec();
}


void MainWindow::openResizeDialog()
{
    if (input->is_vfs_mounted())
        return error(ERR_MOUNTED_VIRTUAL_FS);

    if (input->isEncrypted() && (!input->isCryptoSet() || input->badCrypto()))
    {
        QMessageBox::critical(nullptr,"错误", "密钥缺失或无效 (使用 CTRL+K 配置密钥)");
        return;
    }

    ResizeUserDialog = new ResizeUser(this, input);
    ResizeUserDialog->setWindowTitle("调整 USER");
    ResizeUserDialog->show();
    ResizeUserDialog->exec();
}

void MainWindow::openEmunandDialog()
{
    if (input->is_vfs_mounted())
        return error(ERR_MOUNTED_VIRTUAL_FS);

    EmunandDialog = new Emunand(this, input);
    EmunandDialog->show();
    EmunandDialog->exec();
}

void MainWindow::openDumpDialog(int partition)
{
    DumpDialog = new Dump(this, input, partition);
    DumpDialog->setWindowTitle("高级复制");
    DumpDialog->show();
    //DumpDialog->exec();
}

void MainWindow::openMountDialog()
{
    if (!selected_part)
        return;

    mountDialog = new MountDialog(this, selected_part);
    mountDialog->exec();
    delete mountDialog;
    on_partition_table_itemSelectionChanged();
}

void MainWindow::openDebugDialog()
{
    if(DebugDialog != nullptr)
    {
        if (DebugDialog->isOpen)
        {
            DebugDialog->setFocus();
            return;
        }
        else delete DebugDialog;
    }
    DebugDialog = new Debug(nullptr, isdebug);
    DebugDialog->setWindowTitle("调试控制台");
    DebugDialog->show();
    this->setFocus();
}

void MainWindow::incognito()
{

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Icon::Warning);
    msgBox.setText("擦除序列号将擦除 CAL0 设备的唯一 ID 和 证书");
    msgBox.setInformativeText("!!!警告!!! : 请确保已经备份 PRODINFO 分区用来随时还原 CAL0.\n"
                              "请先备份 PRODINFO 然后选择 \"Yes\" 运行擦除.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();

    if(ret == QMessageBox::Yes)
    {
        //if(bKeyset && parseKeySetFile("keys.dat", &biskeys) >= 2)
        //    input->InitKeySet(&biskeys);
        ret = input->applyIncognito();
        if(ret < 0)
            error(ret);
        else
            QMessageBox::information(nullptr,"擦除","运行擦除序列号成功.");

        //input->InitStorage();
    }

}

void MainWindow::on_rawdump_button_clicked(int crypto_mode, bool rawnand_dump)
{
    if (!crypto_mode) crypto_mode = MD5_HASH;
    QString save_filename(input->getNxTypeAsStr());
    if(rawnand_dump) save_filename = "RAWNAND";
    if(crypto_mode == ENCRYPT)
        save_filename.append(".enc");
    else if (crypto_mode == DECRYPT)
        save_filename.append(".dec");
    else save_filename.append(".bin");


    QString fileName = FileDialog(this, fdMode::save_as, save_filename);
    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (file.exists())
            file.remove();
        file.close();

        // Call WorkerInstance to copy data
        params_t params;
        params.crypto_mode = crypto_mode;
        if(rawnand_dump) params.rawnand_only = true;
        WorkerInstance wi(this, WorkerMode::dump, &params, input, fileName);
        wi.exec();
    }
}

void MainWindow::on_rawdump_button()
{
    on_rawdump_button_clicked(NO_CRYPTO, false);
}

void MainWindow::on_rawdumpDec_button_clicked()
{    
    on_rawdump_button_clicked(DECRYPT, false);
}

void MainWindow::on_rawdumpEnc_button_clicked()
{
    on_rawdump_button_clicked(ENCRYPT, false);
}

void MainWindow::dumpRAWNAND()
{
    on_rawdump_button_clicked(NO_CRYPTO, true);
}
void MainWindow::dumpPartitionAdvanced()
{
    if(!ui->partition_table->selectionModel()->selectedRows().count())
        return;

    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    NxPartition *curPartition = input->getNxPartition(cur_partition.toLocal8Bit().constData());

    if (nullptr == curPartition)
        return;

    openDumpDialog(curPartition->type());
}
void MainWindow::dumpPartition(int crypto_mode)
{

    if(!ui->partition_table->selectionModel()->selectedRows().count())
        return;

    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    NxPartition *curPartition = input->getNxPartition(cur_partition.toLocal8Bit().constData());

    if (nullptr == curPartition)
        return;

    if(crypto_mode == ENCRYPT)
        cur_partition.append(".enc");
    else if (crypto_mode == DECRYPT)
        cur_partition.append(".dec");
    else cur_partition.append(".bin");

    QString fileName = FileDialog(this, fdMode::save_as, cur_partition);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.exists())
        file.remove();
    file.close();

    if(not_in(crypto_mode, {ENCRYPT, DECRYPT}))
        crypto_mode = MD5_HASH;

    // Call WorkerInstance to copy data
    params_t params;
    params.partition = curPartition->type();
    params.crypto_mode = crypto_mode;
    WorkerInstance wi(this, WorkerMode::dump, &params, input, fileName);
    wi.exec();
}

void MainWindow::dumpDecPartition()
{
    dumpPartition(DECRYPT);
}

void MainWindow::dumpEncPartition()
{
    dumpPartition(ENCRYPT);
}

void MainWindow::restorePartition()
{
    if(!ui->partition_table->selectionModel()->selectedRows().count())
        return;

    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    NxPartition *curPartition = input->getNxPartition(cur_partition.toLocal8Bit().constData());

    if (nullptr == curPartition)
        return;

    QString fileName = FileDialog(this, fdMode::open_file);
    if (fileName.isEmpty())
        return;

    selected_io = new NxStorage(fileName.toStdWString());
    if(!selected_io->isNxStorage())
    {
        error(ERR_INPUT_HANDLE, "不是有效的 Nx 存储");
        return;
    }
    selected_part = selected_io->getNxPartition(curPartition->type());
    if(nullptr == selected_part)
    {
        error(ERR_IN_PART_NOT_FOUND);
        return;
    }

    int crypto_mode = NO_CRYPTO;
    // Keyset is provided and restoring native encrypted partition
    if(bKeyset && selected_part->nxPart_info.isEncrypted && not_in(selected_io->setKeys("keys.dat"), { ERR_KEYSET_NOT_EXISTS, ERR_KEYSET_EMPTY }))
    {
        selected_io->setKeys("keys.dat");
        if(!selected_io->badCrypto())
        {
            // Restoring decrypted partition
            if(!selected_part->isEncryptedPartition())
                crypto_mode = ENCRYPT;

            // Restoring encrypted partition to decrypted partition (hummm?)
            else if (selected_part->isEncryptedPartition() && !curPartition->isEncryptedPartition())
                crypto_mode = DECRYPT;
        }
    }

    QString message;
    message.append(QString("您即将恢复分区 %1.\n是否确定继续 ?").arg(QString(selected_part->partitionName().c_str())));

    if(QMessageBox::question(this, "警告", message, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    // DO RESTORE
    params_t params;
    params.partition = curPartition->type();
    WorkerInstance wi(this, WorkerMode::restore, &params, input, "", selected_io);
    wi.exec();
    delete selected_io;
}

void MainWindow::initButtons()
{
	ui->rawdump_button->setText("完全备份");
    if(!input->isNxStorage())
	{
		ui->rawdump_button->setEnabled(false);
		ui->fullrestore_button->setEnabled(false);
	}
    else
    {
        ui->rawdump_button->setEnabled(true);
        ui->fullrestore_button->setEnabled(true);
    }
}
void MainWindow::beforeInputSet()
{
    ui->analysingLbl->setVisible(true);
    ui->partitionsGrp->setVisible(false);
    ui->loadingBar->setVisible(true);

    ui->moreinfo_button->setDisabled(true);

    ui->partQDumpBtn->setDisabled(true);
    ui->partADumpBtn->setDisabled(true);
    ui->partRestoreBtn->setDisabled(true);
    ui->partQDumpBtn->setVisible(false);
    ui->partADumpBtn->setVisible(false);
    ui->partRestoreBtn->setVisible(false);    
    ui->partCustom1Btn->setVisible(false);
    ui->partCustom1Btn->setDisabled(true);

    ui->actionCloseFileDrive->setDisabled(true);
    ui->actionSaveAs->setDisabled(true);
    ui->actionSaveAsAdvanced->setDisabled(true);
    ui->actionDecryptSaveAs->setDisabled(true);
    ui->actionEncryptSaveAs->setDisabled(true);
    ui->actionRestore->setDisabled(true);
    ui->actionProperties->setDisabled(true);
    ui->actionIncognito->setDisabled(true);
    ui->actionautoRCM->setDisabled(true);
    ui->actionResize->setDisabled(true);
    ui->actionDumpRawnandOnly->setDisabled(true);
    ui->actionCreateEmunand->setDisabled(true);

    ui->rawdump_button->setEnabled(false);
    ui->fullrestore_button->setEnabled(false);

    ui->properties_table->setRowCount(0);
    ui->partition_table->setRowCount(0);
    ui->partition_table->setStatusTip(tr(""));

    ui->filedisk_value->setText("");
    ui->nxtype_value->setText("");
    ui->size_value->setText("");
    ui->fwversion_value->setStatusTip("");
    ui->fwversion_value->setText("");
    ui->deviceid_value->setStatusTip("");
    ui->deviceid_value->setText("");
    // Delete mount_button(s)
    for (auto b : ui->selPartGrp->findChildren<QPushButton*>("mount_button"))
        b->deleteLater();
    for (auto b : ui->selPartGrp->findChildren<QPushButton*>("explorer_button"))
        b->deleteLater();
}

void MainWindow::inputSet(NxStorage *storage)
{
    if (!safe_closeInput())
        return;

	input = storage;
    ui->analysingLbl->setVisible(false);    
    ui->loadingBar->setVisible(false);
    ui->partitionsGrp->setVisible(true);

	// Clear table
    ui->partition_table->setRowCount(0);
    ui->partition_table->setStatusTip(tr(""));

    ui->actionCloseFileDrive->setDisabled(false);
    ui->actionSaveAs->setDisabled(true);
    ui->actionSaveAsAdvanced->setDisabled(true);
    ui->actionDecryptSaveAs->setDisabled(true);
    ui->actionEncryptSaveAs->setDisabled(true);
    ui->actionRestore->setDisabled(true);
    ui->actionProperties->setDisabled(true);
    ui->actionIncognito->setDisabled(true);
    ui->actionautoRCM->setDisabled(true);
    ui->actionResize->setDisabled(true);
    ui->actionDumpRawnandOnly->setDisabled(true);
    ui->actionCreateEmunand->setDisabled(true);

    //ui->menuTools->actions().at(5)->setDisabled(false);

    QString path = QString::fromWCharArray(input->m_path), input_label;
    QFileInfo fi(path);
    path = fi.fileName();

    if(input->isDrive() && input->type == RAWMMC)
    {
        if(path.length() > 20)
        {
            path.resize(20);
            path.append("...");
        }
        path.append(" [");
        path.append(n2hexstr(input->mmc_b0_lba_start * NX_BLOCKSIZE, 10).c_str());
        path.append(" -> ");
        path.append(n2hexstr(u64(input->mmc_b0_lba_start * NX_BLOCKSIZE) + storage->size() - 1, 10).c_str());
        path.append("]");
    }
    else if(path.length() > 50)
    {
        path.resize(50);
        path.append("...");
    }
    if (input->isSplitted())
        path.append(" (+" + QString::number(input->nxHandle->getSplitCount() - 1) + ")");

    ui->filedisk_value->setText(path);
    ui->nxtype_value->setText(input->getNxTypeAsStr());
    ui->size_value->setText(QString(GetReadableSize(input->size()).c_str()));
    ui->fwversion_value->setStyleSheet("QLabel { color : #686868; }");
    ui->deviceid_value->setStyleSheet("QLabel { color : #686868; }");
    ui->fwversion_value->setStatusTip("");
    ui->fwversion_value->setText("不适用");
    ui->deviceid_value->setStatusTip("");
    ui->deviceid_value->setText("不适用");

	initButtons();

    if(!input->isNxStorage())
	{
        QString message("导入的文件/驱动器不是有效的 NX 存储."), buff;
        if(input->b_MayBeNxStorage && input->size() <= 0xA0000000)
        {
            message.append("\n确保文件名与分区名一致.\n根据文件大小, 文件名可以是 :\n");
            for( NxPart part : NxPartArr)
            {
                if(part.size == input->size()) {
                    message.append("- ");
                    message.append(part.name);
                    message.append("\n");
                }
            }
        }
        if(input->isSplitted())
            message.append("\n在分卷的备份中找不到 GPT");
		QMessageBox::critical(nullptr,"错误",message);
		return;
	}

    // Save as menu
    ui->actionSaveAs->setEnabled(true);
    ui->actionSaveAsAdvanced->setEnabled(true);

    if(input->isSinglePartType())
    {
        NxPartition *part = input->getNxPartition();
        // Decrypt & save as menu
        if(part->isEncryptedPartition() && bKeyset && !part->badCrypto())
            ui->actionDecryptSaveAs->setEnabled(true);
        // Encrypt & save as menu
        if(!part->isEncryptedPartition() && bKeyset && part->nxPart_info.isEncrypted)
            ui->actionEncryptSaveAs->setEnabled(true);
    }

    // Restore from file
    ui->actionRestore->setEnabled(true);

    // Properties menu
    ui->actionProperties->setEnabled(true);

    // Incognito menu
    NxPartition *cal0 = input->getNxPartition(PRODINFO);
    if(nullptr != cal0 && (!cal0->isEncryptedPartition() || (bKeyset && !cal0->badCrypto())))
        ui->actionIncognito->setEnabled(true);

    // AutoRcm menu
    if(nullptr != input->getNxPartition(BOOT0) && input->isEristaBoot0)
        ui->actionautoRCM->setEnabled(true);

    // Resize NAND menu
    if(is_in(input->type, {RAWNAND, RAWMMC}))
    {
        ui->actionResize->setEnabled(true);
        ui->actionCreateEmunand->setEnabled(true);
    }
    if(input->type == RAWMMC)
        ui->actionDumpRawnandOnly->setEnabled(true);

    // Fill partition TableWidget
    for (NxPartition *part : input->partitions)
    {
        // Add new row
        ui->partition_table->insertRow(ui->partition_table->rowCount());
        int index = ui->partition_table->rowCount() - 1;
        ui->partition_table->setItem(index, 0, new QTableWidgetItem(QString(part->partitionName().c_str())));
        //ui->partition_table->setItem(index, 1, new QTableWidgetItem(GetReadableSize(part->size()).c_str()));
        //ui->partition_table->setItem(index, 2, new QTableWidgetItem(part->isEncryptedPartition() ? "是" : "否"));
    }
    ui->partition_table->resizeColumnsToContents();
    ui->partition_table->resizeRowsToContents();
    ui->partition_table->setStatusTip(tr("右键单击分区 备份/还原/文件."));

    if (ui->partition_table->rowCount())
        ui->partition_table->setCurrentIndex(ui->partition_table->model()->index(0, 0));

    // Display storage information
    if(input->firmware_version.major > 0)
    {
        ui->fwversion_value->setText(QString::fromStdString(input->getFirmwareVersion()).append(input->exFat_driver ? " (exFAT)" : ""));
    }
    else
    {
        NxPartition *system = input->getNxPartition(SYSTEM);
        if(nullptr != system)
        {
            if(system->badCrypto()) {
                ui->fwversion_value->setText("密钥损坏!");
                ui->fwversion_value->setStyleSheet("QLabel { color : red; }");
                ui->fwversion_value->setStatusTip("解密内容时出错, 密钥错误 ? (CTRL+K 配置密钥)");
            }
            else {

                bool search = false;
                if (!m_ncaDB->is_Empty())
                {
                    // Look for firmware version in NCA DB (async)
                    search = true;
                    QFuture<void> future = QtConcurrent::run([&]() {
                        auto system = input->getNxPartition(SYSTEM);
                        bool found = false;
                        if (system->mount_fs() == SUCCESS)
                        {
                            for (auto t : m_ncaDB->findTitlesById("0100000000000809")) //SystemVersion
                            {
                                auto file = NxFile(system, wstring(L"/Contents/registered/").append(t->filename.toStdWString()));
                                if (file.exists()) {
                                    ui->fwversion_value->setText(t->firmware);
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (!found) {
                            ui->fwversion_value->setText("未找到!");
                            ui->fwversion_value->setStatusTip("无法找到 fw 版本");
                        }
                    });
                }

                if (!search)
                {
                    ui->fwversion_value->setText("未找到!");
                    ui->fwversion_value->setStatusTip("无法找到 fw 版本 (CTRL+K 配置密钥)");
                }
                else ui->fwversion_value->setText("搜索...");

            }
        }
        else ui->fwversion_value->setText("不适用");
    }


    if (strlen(input->deviceId))
        ui->deviceid_value->setText(input->deviceId);
    else if (nullptr != cal0)
    {
        if(cal0->badCrypto()) {
            ui->deviceid_value->setText("密钥损坏!");
            ui->deviceid_value->setStyleSheet("QLabel { color : red; }");
            ui->deviceid_value->setStatusTip("解密内容时出错, 密钥错误 ? (CTRL+K 配置密钥)");
        }
        else {
            ui->deviceid_value->setText("需要密钥!");
            ui->deviceid_value->setStatusTip("无法解密内容 (CTRL+K 配置密钥)");
        }
    }
    else ui->deviceid_value->setText("不适用");

    ui->moreinfo_button->setEnabled(true);
}


void MainWindow::on_partition_table_itemSelectionChanged()
{

    if(!ui->partition_table->selectionModel()->selectedRows().count())
        return;


	// Partition table context menu
	foreach (QAction *action, ui->partition_table->actions()) {
		ui->partition_table->removeAction(action);
        delete action;
	}
    ui->partCustom1Btn->disconnect();

    ui->partition_table->setContextMenuPolicy(Qt::ActionsContextMenu);

    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    if (!(selected_part = input->getNxPartition(cur_partition.toLocal8Bit().constData())))
        return;

    // Set buttons visibility
    ui->partQDumpBtn->setVisible(true);
    ui->partADumpBtn->setVisible(true);
    ui->partRestoreBtn->setVisible(true);
    ui->partQDumpBtn->setEnabled(true);
    ui->partADumpBtn->setEnabled(true);
    ui->partRestoreBtn->setEnabled(true);
    ui->partCustom1Btn->setVisible(false);
    ui->partCustom1Btn->setEnabled(false);

    // Dump action
    QAction* dumpAction = new QAction(dumpIcon, "备份文件...");
    dumpAction->setStatusTip(tr("新文件另存为"));
    ui->partition_table->connect(dumpAction, SIGNAL(triggered()), this, SLOT(dumpPartition()));
    ui->partition_table->addAction(dumpAction);    

    // Dump advanced action
    QAction* dumpAAction = new QAction(dumpIcon, "备份 (高级)");
    dumpAAction->setStatusTip(tr("备份 - 高级选项"));
    ui->partition_table->connect(dumpAAction, SIGNAL(triggered()), this, SLOT(dumpPartitionAdvanced()));
    ui->partition_table->addAction(dumpAAction);

    // Restore action
    QAction* restoreAction = new QAction(restoreIcon, "从文件还原...");
    restoreAction->setStatusTip(tr("打开现有文件"));
    ui->partition_table->connect(restoreAction, SIGNAL(triggered()), this, SLOT(restorePartition()));
    ui->partition_table->addAction(restoreAction);

    // Decrypt action
    if(selected_part->isEncryptedPartition() && !selected_part->badCrypto())
    {
        QAction* dumpDecAction = new QAction(encIcon, "解密和&备份文件...");
        dumpDecAction->setStatusTip(tr("新文件另存为"));
        if(!bKeyset)
            dumpDecAction->setDisabled(true);
        ui->partition_table->connect(dumpDecAction, SIGNAL(triggered()), this, SLOT(dumpDecPartition()));
        ui->partition_table->addAction(dumpDecAction);
    }

    // Encrypt action
    if(selected_part->nxPart_info.isEncrypted && !selected_part->isEncryptedPartition())
    {
        QAction* dumpEncAction = new QAction(decIcon, "加密和&备份文件...");
        dumpEncAction->setStatusTip(tr("新文件另存为"));
        if(!bKeyset)
            dumpEncAction->setDisabled(true);
        ui->partition_table->connect(dumpEncAction, SIGNAL(triggered()), this, SLOT(dumpEncPartition()));
        ui->partition_table->addAction(dumpEncAction);
    }

    // AutoRCM action
    if (selected_part->type() == BOOT0 && input->isEristaBoot0)
    {
        ui->partition_table->setContextMenuPolicy(Qt::ActionsContextMenu);
        QString statusTip(tr(input->autoRcm ? "禁用 autoRCM" : "启用 AutoRCM"));
        QAction* action = new QAction(rcmIcon, statusTip);
        action->setStatusTip(statusTip);
        ui->partition_table->connect(action, SIGNAL(triggered()), this, SLOT(toggleAutoRCM()));
        ui->partition_table->addAction(action);

        ui->partCustom1Btn->setIcon(rcmIcon);
        ui->partCustom1Btn->setStatusTip(statusTip);
        ui->partCustom1Btn->setToolTip(statusTip);
        ui->partCustom1Btn->connect(ui->partCustom1Btn, SIGNAL(clicked()), this, SLOT(toggleAutoRCM()));
        ui->partCustom1Btn->setVisible(true);
        ui->partCustom1Btn->setEnabled(true);
    }

    // Incognito action
    if (selected_part->type() == PRODINFO)
    {
        QAction* incoAction = new QAction(incoIcon, "应用擦除序列号");
        QString statusTip(tr("擦除 PRODINFO 中的个人信息"));
        incoAction->setStatusTip(statusTip);
        ui->partition_table->connect(incoAction, SIGNAL(triggered()), this, SLOT(incognito()));
        ui->partition_table->addAction(incoAction);

        ui->partCustom1Btn->setIcon(incoIcon);
        ui->partCustom1Btn->setStatusTip(statusTip);
        ui->partCustom1Btn->setToolTip(statusTip);
        ui->partCustom1Btn->connect(ui->partCustom1Btn, SIGNAL(clicked()), this, SLOT(incognito()));
        ui->partCustom1Btn->setVisible(true);
        ui->partCustom1Btn->setEnabled(true);
    }

    // Format partition action
    if (selected_part->type() == USER || selected_part->type() == SYSTEM)
    {
        QAction* formtAction = new QAction(formtIcon, "分区格式化 (FAT32)");
        QString statusTip(tr("删除所选分区所有数据 (快速格式化)"));
        formtAction->setStatusTip(statusTip);
        ui->partition_table->connect(formtAction, SIGNAL(triggered()), this, SLOT(formatPartition()));
        ui->partition_table->addAction(formtAction);

        ui->partCustom1Btn->setIcon(formtIcon);
        ui->partCustom1Btn->setStatusTip(statusTip);
        ui->partCustom1Btn->setToolTip(statusTip);
        ui->partCustom1Btn->connect(ui->partCustom1Btn, SIGNAL(clicked()), this, SLOT(formatPartition()));
        ui->partCustom1Btn->setVisible(true);
        ui->partCustom1Btn->setEnabled(true);        
    }

    // Explorer action
    auto explorer_button = ui->selPartGrp->findChild<QPushButton*>("explorer_button");
    if (explorer_button)
        delete explorer_button;

    if(is_in(selected_part->type(), {USER, SYSTEM}))
    {
        QString statusTip(tr("搜索分区 (保存 & 已安装 titles)"));
        if (!selected_part->isGood())
            statusTip.append(selected_part->crypto() ? " CRYPTO 失败! 密钥错误" : " 密钥丢失! CTRL+K 配置密钥");

        auto explorer_button = new QPushButton(this);
        explorer_button->setObjectName("explorer_button");
        explorer_button->setFixedSize(30, 30);
        explorer_button->setIcon(explorerIcon);
        explorer_button->setStatusTip(statusTip);
        if (selected_part->isGood())
            connect(explorer_button, &QPushButton::clicked, this, &MainWindow::openExplorer);
        else
            explorer_button->setDisabled(true);
        ui->horizontalLayout_2->insertWidget(ui->horizontalLayout_2->count()-1, explorer_button);

        QAction* explAction = new QAction(explorerIcon, "搜索分区");
        explAction->setStatusTip(statusTip);
        ui->partition_table->connect(explAction, &QAction::triggered, this, &MainWindow::openExplorer);
        ui->partition_table->addAction(explAction);
        if (!selected_part->isGood())
            explAction->setDisabled(true);
    }

    // Mount action
    auto button = ui->selPartGrp->findChild<QPushButton*>("mount_button");
    if (button)
        delete button;

    if (is_in(selected_part->type(), {USER, SYSTEM, SAFE, PRODINFOF}))
    {
        auto button = new QPushButton(this);
        button->setObjectName("mount_button");
        button->setFixedSize(110, 30);
        connect(button, &QPushButton::clicked, [=]() {
            if(selected_part->is_vfs_mounted())
                on_mountParition(selected_part->type());
            else
                openMountDialog(); //mountContextMenu();
        });
        ui->horizontalLayout_2->insertWidget(ui->horizontalLayout_2->count()-1, button);

        button->setIcon(selected_part->is_vfs_mounted() ? unmountIcon : mountIcon);
        QString label = selected_part->is_vfs_mounted() ? "弹出" : "挂载";
        if(selected_part->is_vfs_mounted())
        {
            WCHAR mountPoint[3] = L" \0";
            selected_part->getVolumeMountPoint(mountPoint);
            label.append(" (" + QString::fromStdWString(mountPoint).toUpper() + ":)");
        }

        QString statusTip(selected_part->is_vfs_mounted() ? "弹出虚拟磁盘" : "将分区挂载为虚拟磁盘 (虚拟文件系统)");
        if (selected_part->isEncryptedPartition() && (selected_part->badCrypto() || !selected_part->crypto()))
        {
            button->setDisabled(true);
            statusTip = selected_part->crypto() ? "密钥出错! 密钥错误" : "密钥丢失! CTRL+K 配置密钥";
        }
        else
        {
            button->setEnabled(true);
            QAction* mountAction = new QAction(selected_part->is_vfs_mounted() ? unmountIcon : mountIcon, label);
            mountAction->setStatusTip(statusTip);
            ui->partition_table->connect(mountAction, &QAction::triggered, [=]() {
                if (selected_part->is_vfs_mounted())
                    on_mountParition(selected_part->type());
                else openMountDialog();
            });
            ui->partition_table->addAction(mountAction);

            if (!selected_part->is_vfs_mounted()) {
                QAction* quickMountAction = new QAction(mountIcon, "快速挂载");
                quickMountAction->setStatusTip("快速挂载");
                ui->partition_table->connect(quickMountAction, &QAction::triggered, [=]() {
                    if (m_vfsRunner != nullptr)
                        delete m_vfsRunner;

                    m_vfsRunner = new VfsMountRunner(selected_part);
                    VfsMountRunner runner(selected_part);
                    connect(m_vfsRunner, &VfsMountRunner::error, this, &MainWindow::error);
                    connect(m_vfsRunner, &VfsMountRunner::mounted, [=](){
                        QMessageBox::information(this, "成功", QString("挂载的分区 (%1).").arg(m_vfsRunner->mounPoint()));
                        on_partition_table_itemSelectionChanged();
                    });
                    m_vfsRunner->run();
                });
                ui->partition_table->addAction(quickMountAction);
            }
        }
        button->setStatusTip(statusTip);
        button->setToolTip(statusTip);
        button->setText(label);
    }

    // Clear properties table
    ui->properties_table->setRowCount(0);

    // Add new property lambda
    auto addItem = [&](QString key, QString value = "") {
        auto ix = ui->properties_table->rowCount();
        ui->properties_table->insertRow(ix);

        auto wdg_1 = new QTableWidgetItem(key);
        wdg_1->setTextAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
        ui->properties_table->setItem(ix, 0, wdg_1);

        if (!value.length())
        {
            ui->properties_table->setSpan(ix, 0, 1, 2);
            ui->properties_table->resizeRowToContents(ix);
            return;
        }
        auto wdg_2 = new QTableWidgetItem(value);
        wdg_2->setTextAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
        ui->properties_table->setItem(ix, 1, wdg_2);
        ui->properties_table->setRowHeight(ix, 20);

    };
    // Fill properties table
    QString fs;
    switch (selected_part->nxPart_info.fs) {
        case FAT12 : fs = "FAT12";
            break;
        case FAT32 : fs = "FAT32";
            break;
        default : fs = "无 (RAW)";
    }
    addItem("文件系统:", fs);
    addItem(selected_part->availableTotSpace ? "RAW 大小:" : "大小:",
            QString::fromStdString(GetReadableSize(selected_part->size())));
    if (selected_part->availableTotSpace)
    {
        addItem("可用空间:", QString::fromStdString(GetReadableSize(selected_part->availableTotSpace)));
        addItem("空闲空间:", QString::fromStdString(GetReadableSize(selected_part->freeSpace)));
    }
    addItem("起始扇区:", QString::number(selected_part->lbaStart())
                             + " (" + QString::fromStdString(int_to_hex(selected_part->lbaStart()) + ")"));
    addItem("最后扇区:", QString::number(selected_part->lbaEnd()) + " ("
                            + QString::fromStdString(int_to_hex(selected_part->lbaEnd()) + ")"));
    addItem("加密:", selected_part->isEncryptedPartition() ? "是" : "否");

    if (selected_part->type() == BOOT0)
    {
        addItem("Soc 修订:", input->isEristaBoot0 ? "Erista" : "未知 (Mariko ?)");
        if (input->isEristaBoot0)
        {
            addItem("AutoRCM:", input->autoRcm ? "启用" : "禁用");
            addItem("Bootloader 版本:", QString::number(input->bootloader_ver));
        }
    }
    QString info;
    switch (selected_part->type())
    {
    case BOOT0:
        info = "- BCT - 第一个 bootloader (package1ldr)\n- 第二个 bootloader (package1)\n- TrustZone code";
        break;
    case BOOT1:
        info = "包含 safe mode package1 (参照 BOOT0)";
        break;
    case PRODINFO:
        info = "CAL0. 包含主要校准数据的原始二进制 blob, 其中包括硬件ID, 系统密钥";
        break;
    case PRODINFOF:
        info = "包含额外的校准数据.";
        break;
    case BCPKG21:
        info = "- BootConfig\n- Switch 内核 & 系统模块";
        break;
    case BCPKG22:
        info = "备份分区为 BCPKG2-1-Normal-Main";
        break;
    case BCPKG23:
        info = "包含 safe mode package2";
        break;
    case BCPKG24:
        info = "备份分区为 BCPKG2-3-SafeMode-Main";
        break;
    case BCPKG25:
        info = "安装于工厂, 之后从未用于零售";
        break;
    case BCPKG26:
        info = "备份分区为 BCPKG2-5-Repair-Main";
        break;
    case SAFE:
        info = "这个分区的正式名称是 \"SafeMode\"";
        break;
    case SYSTEM:
        info = "- 系统 titles (应用程序)\n- 保存为系统 titles";
        break;
    case USER:
        info = "- 非系统 titles (游戏, 应用程序)\n- 保存为非系统 titles";
        break;

    }
    addItem("说明:\n" + info);
}

void MainWindow::driveSet(QString drive)
{    
	qApp->processEvents();
    if (!safe_closeInput())
        return;

	// Open new thread to init storage (callback => inputSet(NxStorage))
    beforeInputSet();
    if (workThread)
        delete workThread;
    workThread = new Worker(this,  WorkerMode::new_storage, drive);
	workThread->start();
}

void MainWindow::resizeUser(QString file, int new_size, bool format)
{
    selected_io = nullptr;
    // Open new thread
    //workThread = new Worker(this, input, file, new_size, format);
    //startWorkThread();
}

void MainWindow::openExplorer()
{
    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    NxPartition *curPartition = input->getNxPartition(cur_partition.toLocal8Bit().constData());

    if (nullptr == curPartition)
        return;

    ExplorerDialog = new Explorer(this, curPartition);
    //ExplorerDialog->setWindowTitle("浏览");
    ExplorerDialog->show();
    ExplorerDialog->exec();
    delete ExplorerDialog;

}

void MainWindow::error(int err, QString label)
{
	if(err != ERR_WORK_RUNNING)
	{
		if(label != nullptr)
		{
            QMessageBox::critical(nullptr,"错误", label);
			return;
		}
	}

	for (int i=0; i < (int)array_countof(ErrorLabelArr); i++)
	{
		if(ErrorLabelArr[i].error == err) {
            QMessageBox::critical(nullptr,"错误", QString(ErrorLabelArr[i].label));
			return;
		}
	}

    QMessageBox::critical(nullptr,"错误","错误 " + QString::number(err));
}


void MainWindow::on_fullrestore_button_clicked()
{

	// Create new file dialog
    QString fileName = FileDialog(this, fdMode::open_file);
    if (fileName.isEmpty())
        return;

    //New input storage
    selected_io = new NxStorage(fileName.toStdWString());

    if(!selected_io->isNxStorage())
    {
        error(ERR_INPUT_HANDLE, "不是有效的 Nx 存储");
        return;
    }

    if(input->isSinglePartType() && nullptr == selected_io->getNxPartition(input->getNxTypeAsInt()))
    {
        error(ERR_IN_PART_NOT_FOUND);
        return;
    }
    /*
    if(!input->isSinglePartType() && ( selected_io->type != input->type ||  selected_io->size() > input->size()))
    {
        error(ERR_IO_MISMATCH);
        return;
    }
    */
    QString message;
    message.append("您即将还原到现有状态 " + QString(input->isDrive() ? "驱动器" : "文件") + "\n是否确定继续 ?");
    if(QMessageBox::question(this, "警告", message, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    //WorkParam_t param;

    // Partition restore
    params_t params;
    if(input->isSinglePartType())
    {
        params.partition = input->getNxTypeAsInt();
        /*
         * TODO
         * Put this into NxStorage !
         *
        NxPartition *out_part = input->getNxPartition();
        NxPartition *selected_part = selected_io->getNxPartition(input->getNxTypeAsInt());
        int crypto_mode = NO_CRYPTO;
        // Keyset is provided and restoring native encrypted partition
        if(bKeyset && selected_part->nxPart_info.isEncrypted && not_in(selected_io->setKeys("keys.dat"), { ERR_KEYSET_NOT_EXISTS, ERR_KEYSET_EMPTY }))
        {
            selected_io->setKeys("keys.dat");
            if(!selected_io->badCrypto())
            {
                // Restoring decrypted partition
                if(!selected_part->isEncryptedPartition())
                    crypto_mode = ENCRYPT;

                // Restoring encrypted partition to decrypted partition (hummm?)
                else if (selected_part->isEncryptedPartition() && !out_part->isEncryptedPartition())
                    crypto_mode = DECRYPT;
            }
        }
        */
    }
    WorkerInstance wi(this, WorkerMode::restore, &params, input, "", selected_io);
    wi.exec();
    delete selected_io;

}

void MainWindow::toggleAutoRCM()
{
	bool pre_autoRcm = input->autoRcm;

    if(input->type == RAWMMC && !pre_autoRcm && QMessageBox::question(this, "警告", "请注意, 无法在虚拟系统上激活 autoRCM, 因为它只在 sysNAND 上工作.\n是否确定继续 ?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        QMessageBox::information(this, "信息", "已取消");
        return;
    }

    if(!input->setAutoRcm(input->autoRcm ? false : true))
		QMessageBox::critical(nullptr,"错误", "切换 autoRCM 时出错");
    else {
		QMessageBox::information(this, "成功", "AutoRCM 已"  + QString(input->autoRcm ? "启用" : "禁用"));
        qApp->processEvents();
        beforeInputSet();
        QString filename = QString::fromWCharArray(input->m_path);
        if (!safe_closeInput())
            return;
        if (workThread)
            delete workThread;
        workThread = new Worker(this, WorkerMode::new_storage, filename);
        workThread->start();

    }
}

void MainWindow::formatPartition()
{
    if(!ui->partition_table->selectionModel()->selectedRows().count())
        return;

    QString cur_partition(ui->partition_table->selectedItems().at(0)->text());
    NxPartition *curPartition = input->getNxPartition(cur_partition.toLocal8Bit().constData());

    if (nullptr == curPartition)
        return;

    auto text = QString("格式化将删除 %1 的所有数据.")
            .arg(QString::fromStdString(curPartition->partitionName()));
    if (curPartition->is_vfs_mounted())
        text.append(QString("\n虚拟磁盘 (%1) 将首先被卸载.")
                    .arg(QString::fromStdWString(curPartition->vfs()->mount_point).toUpper()));
    text.append("\n是否确定继续 ?");

    if(QMessageBox::question(this, "警告", text, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    auto res = curPartition->formatPartition();
    if (res)
        QMessageBox::critical(this, "错误", "格式化分区失败");
    else
        QMessageBox::information(this, "信息", "分区形成");

    // Reopen NxStorage
    beforeInputSet();
    QString filename = QString::fromWCharArray(input->m_path);
    if (!safe_closeInput())
        return;
    if (workThread)
        delete workThread;
    workThread = new Worker(this, WorkerMode::new_storage, filename);
    workThread->start();

}

void MainWindow::keySetSet()
{
    bKeyset = false;
    QFile file("keys.dat");
    if (file.exists() && parseKeySetFile("keys.dat", &biskeys) >= 2)
        bKeyset = true;

    if(nullptr != input && input->type != UNKNOWN && input->type != INVALID)
    {
        qApp->processEvents();
        QString filename = QString::fromWCharArray(input->m_path);
        if (!safe_closeInput())
            return;
        if (workThread)
            delete workThread;
        workThread = new Worker(this, WorkerMode::new_storage, filename);
        workThread->start();
    }
}

void MainWindow::on_moreinfo_button_clicked()
{
    if(nullptr != input && input->type != INVALID && input->type != UNKNOWN)
        Properties();
}

void MainWindow::on_rawdump_button_clicked()
{
    on_rawdump_button_clicked(NO_CRYPTO, false);
}

void MainWindow::on_mountParition(int nx_type, const wchar_t &mount_point)
{
    if(!input)
        return;

    auto mount_button = ui->selPartGrp->findChild<QPushButton*>("mount_button");
    mount_button->setDisabled(true);
    auto exit = [&](int e, const QString l = nullptr ){
        if (e)
            emit error(e, l);
        mount_button->setEnabled(true);
        emit on_partition_table_itemSelectionChanged();
        return;
    };

    NxPartition *nxp = input->getNxPartition(nx_type);
    if (!nxp)
        return exit(ERR_IN_PART_NOT_FOUND);

    if (mount_button)
        mount_button->setText(nxp->is_vfs_mounted() ? "弹出中..." : "挂载中...");

    // Unmount
    if(nxp->is_vfs_mounted())
        return exit(nxp->unmount_vfs());

    nxp->disconnect();
    connect(nxp, &NxPartition::vfs_mounted_signal, this, &MainWindow::on_partition_table_itemSelectionChanged);
    connect(nxp, &NxPartition::vfs_callback, [&](long status){
        if (status == DOKAN_DRIVER_INSTALL_ERROR)
            emit dokanDriver_install_signal();
        else if (status < -1000)
            emit error_signal((int)status);
        else if (status != DOKAN_SUCCESS)
            emit error_signal(1, QString::fromStdString(dokanNtStatusToStr(status)));

        emit this->on_partition_table_itemSelectionChanged();
    });
    QtConcurrent::run(nxp, &NxPartition::mount_vfs, true, mount_point, m_isMountOptionReadOnly ? ReadOnly : VirtualNXA, nullptr);

}

void MainWindow::mountContextMenu()
{
    auto button = ui->selPartGrp->findChild<QPushButton*>("mount_button");
    if (!button)
        return;

    auto mount_points = GetAvailableMountPoints();
    if (!mount_points.size())
        emit error(1, "未找到任何可用的挂载目录");

    QMenu contextMenu("挂载分区", this);
    QAction action1(QString("自动. 挂载点 (%1:)").arg(QString(mount_points.at(0)).toUpper()), this);
    connect(&action1, &QAction::triggered, [&]() {
        on_mountParition(selected_part->type(), mount_points.at(0));
    });
    contextMenu.addAction(&action1);

    auto subContextMenu = contextMenu.addMenu("选择挂载目录");
    for (const auto mount_point : mount_points)
    {
        auto action = QString(mount_point).toUpper() + ":";
        auto subAction = new QAction (action, this);
        connect(subAction, &QAction::triggered, [=]() {
            on_mountParition(selected_part->type(), mount_point);
        });
        subContextMenu->addAction(subAction);
    }
    contextMenu.addSeparator();
    QAction readOnlyAction("挂载为只读", this);
    readOnlyAction.setCheckable(true);
    readOnlyAction.setChecked(m_isMountOptionReadOnly);
    connect(&readOnlyAction, &QAction::triggered, [&]() {
        m_isMountOptionReadOnly = readOnlyAction.isChecked();
    });
    contextMenu.addAction(&readOnlyAction);
    contextMenu.exec(button->mapToGlobal(button->rect().bottomLeft()));
}

void MainWindow::dokanDriver_install()
{
    if(QMessageBox::question(nullptr, "错误", "未找到 Dokan 驱动程序\n是否继续安装 ?",
             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        int res = installDokanDriver();
        if (res)
            emit error(res);
    }
}

void MainWindow::launch_vfs(virtual_fs::virtual_fs* fs)
{
    if(fs->populate() < 0)
    {
        emit error(ERR_FAILED_TO_POPULATE_VFS);
        return;
    }
    fs->run();
}

void MainWindow::restartDebug()
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    BOOL ret = FALSE;
    DWORD flags = CREATE_NO_WINDOW;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    wchar_t buffer[_MAX_PATH];
    GetModuleFileName(GetCurrentModule(), buffer, _MAX_PATH);
    wstring module_path(buffer);
    module_path.append(L" --gui DEBUG_MODE");
    ret = CreateProcess(nullptr, &module_path[0], nullptr, nullptr, NULL, flags, nullptr, nullptr, &si, &pi);
    exit(EXIT_SUCCESS);
}
