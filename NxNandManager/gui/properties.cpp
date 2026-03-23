/*
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

#include "ui_properties.h"
#include "properties.h"
#include "../NxSave.h"

Properties::Properties(NxStorage *in) :
    ui(new Ui::DialogProperties)
{
    ui->setupUi(this);
    input = in;
    int i = 0;
    char buffer[0x100];

    ui->PropertiesTable->setRowCount(0);
    ui->PropertiesTable->setColumnCount(2);
    ui->PropertiesTable->setColumnWidth(0, 100);
    ui->PropertiesTable->setColumnWidth(1, 220);
    QStringList header;
    header<<"属性"<<"值";
    ui->PropertiesTable->setHorizontalHeaderLabels(header);
    QFont font("Calibri", 10, QFont::Bold);
    ui->PropertiesTable->horizontalHeader()->setFont(font);
    ui->PropertiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    //ui->PropertiesTable->setWordWrap(false);
    //ui->PropertiesTable->setTextElideMode(Qt::ElideNone);



    wstring ws(input->m_path);
    ui->PropertiesTable->setRowCount(i+1);
    ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("路径"));
    ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(string(ws.begin(), ws.end()).c_str()));
    i++;

    sprintf(buffer, "%s%s", input->getNxTypeAsStr(), input->isSplitted() ? " (分卷备份)" : "");
    ui->PropertiesTable->setRowCount(i+1);
    ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("NAND 类型"));
    ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(QString(buffer).trimmed()));
    i++;

    ui->PropertiesTable->setRowCount(i+1);
    ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("文件/磁盘"));
    ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->isDrive() ? "磁盘" : "文件"));
    i++;


    sprintf(buffer, "%s%s", input->isEncrypted() ? "是" : "否",
                    input->isEncrypted() && input->badCrypto() ? "  !!! 解密失败 !!!" : "");
    ui->PropertiesTable->setRowCount(i+1);
    ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("加密"));
    ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(QString(buffer).trimmed()));
    i++;

    ui->PropertiesTable->setRowCount(i+1);
    ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("大小"));
    ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(GetReadableSize(input->size()).c_str()));
    i++;

    NxPartition *boot0 = input->getNxPartition(BOOT0);
    if (nullptr != boot0)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("SoC 修订"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->isEristaBoot0 ? "Erista" : "未知 (Mariko ?)"));
        i++;

        if (input->isEristaBoot0)
        {
            ui->PropertiesTable->setRowCount(i+1);
            ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("Auto RCM"));
            ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->autoRcm ? "启用" : "禁用"));
            i++;

            ui->PropertiesTable->setRowCount(i+1);
            ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("Bootloader 版本"));
            sprintf(buffer, "%d", static_cast<int>(input->bootloader_ver));
            ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(QString(buffer).trimmed()));
            i++;
        }
    }

    if(strlen(input->fw_version))
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("固件版本"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->fw_version));
        i++;

        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("ExFat 驱动"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->exFat_driver ? "检测到" : "未检测到"));
        i++;
    }
    /*
    if (strlen(input->last_boot) > 0)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("Last boot time"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->last_boot));
        i++;
    }
    */
    if (strlen(input->serial_number) > 3)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("序列号"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->serial_number));
        i++;
    }

    if (strlen(input->deviceId) > 0)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("设备 ID"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->deviceId));
        i++;
    }

    //if (strlen(input->wlanMacAddress) > 0)
    if (input->macAddress.length() > 0)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("MAC 地址"));
        //ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(hexStr(reinterpret_cast<unsigned char*>(input->wlanMacAddress), 6).c_str()));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(input->macAddress.c_str()));
        i++;
    }

    if (input->type == RAWNAND || input->type == RAWMMC)
    {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("GPT 备份"));
        if(input->backupGPT())
        {
            sprintf(buffer, "找到 (偏移 0x%s)", n2hexstr(input->backupGPT(), 10).c_str());
            ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem(QString(buffer).trimmed()));
        }
        else
            ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem("未找到"));
        i++;
    }

    auto system = input->getNxPartition(SYSTEM);
    if (!system || system->mount_fs() != SUCCESS)
        return;
    NxSave settings(system, L"/save/8000000000000050");
    if (!settings.exists() || !settings.open())
        return;

    NxSaveFile file;
    if (!settings.getSaveFile(&file, "/file"))
        return;

    u8 device_nickname[0x16];
    if (settings.readSaveFile(file, device_nickname, 0x2A950, 0x16) == 0x16) {
        ui->PropertiesTable->setRowCount(i+1);
        ui->PropertiesTable->setItem(i, 0, new QTableWidgetItem("设备昵称"));
        ui->PropertiesTable->setItem(i, 1, new QTableWidgetItem((const char*)device_nickname));
        i++;
    }

}

Properties::~Properties()
{
    delete ui;
}

void Properties::on_DialogProperties_finished(int result)
{
    isOpen = false;
}
