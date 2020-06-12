#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "reg.h"
#include "file.h"
#include "sig.h"
#include "convert.h"
#include "task.h"

#define logOnList { "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",                  \
                    "HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",     \
                    "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"}

#define servicesReg "HKLM\\SYSTEM\\CurrentControlSet\\Services"
#define knownDLLsReg "HKLM\\System\\CurrentControlSet\\Control\\Session Manager\\KnownDlls"

struct HKEY_PATH
{
    HKEY hKey;
    LPCSTR path;
};

void InitLogOnTable(Ui::MainWindow *ui, QString reg);
void InitServicesTable(Ui::MainWindow *ui, QString reg);
void RepairString(QString *str);
void InitLogOnTable(QTableWidget *table, QString reg);
void InitServicesTable(QTableWidget *table, QString reg);
void InitDriversTable(QTableWidget *table, QString reg);
void InitTasksTable(QTableWidget *table);
void InitKnownDllsTable(QTableWidget *table, QString reg);
void InitTableColumnWidth(QTableWidget* table);

void getHkeyAndPath(QString reg, HKEY_PATH *item);
void myReadReg(HKEY hKey, LPCSTR path, QMap<QString, QString> *map);
void ReadServices(HKEY hKey, LPCSTR path, QMap<QString, QString> *map);
void ReadTasks(QMap<QString, QString> *map);
void DrawTable(QTableWidget *table, int rowIndex, QString imagePath, QString description);
void DrawHeader(QTableWidget *table, int rowIndex, QString reg);
void ShowDetail(int row, int column, QTableWidget *table, Ui::MainWindow *ui);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    foreach(QString logOnReg, QStringList logOnList)
    {
        InitLogOnTable(ui->tableWidget_logOn, logOnReg);
    }
    InitServicesTable(ui->tableWidget_services, servicesReg);
    InitDriversTable(ui->tableWidget_drivers, servicesReg);
    InitTasksTable(ui->tableWidget_tasks);
    InitKnownDllsTable(ui->tableWidget_knownDLLs, knownDLLsReg);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
    QStackedWidget* stack = ui->stackedWidget;
    stack->setCurrentIndex(currentRow);
}

void InitTableColumnWidth(QTableWidget* table)
{
    // initialize the column width
    table->setGeometry(10, 0, 1080, 720);
    table->setColumnWidth(0, 20);
    table->setColumnWidth(1, 180);
    table->setColumnWidth(2, 180);
    table->setColumnWidth(3, 180);
    table->setColumnWidth(4, 300);
//    table->setColumnWidth(4,420);
//    table->setColumnWidth(5,200);
}

void RepairString(QString *str)
{
    if (str->contains(" /"))
        *str = str->split(" /")[0];
    if (str->contains(" -"))
        *str = str->split(" -")[0];
    if (str->contains("\""))
        *str = str->replace("\"", "");
    if (str->contains("\\??\\"))
        *str = str->replace("\\??\\", "");
    if (str->contains("%windir%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("%windir%", 0, Qt::CaseInsensitive), 8, "C:\\Windows");
    if (str->contains("@%systemroot%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("@%systemroot%", 0, Qt::CaseInsensitive), 13, "C:\\Windows");
    if (str->contains("%systemroot%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("%systemroot%", 0, Qt::CaseInsensitive), 12, "C:\\Windows");
    if (str->contains("\\SystemRoot", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("\\SystemRoot", 0, Qt::CaseInsensitive), 11, "C:\\Windows");
    if (str->startsWith("system32", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("system32", 0, Qt::CaseInsensitive), 8, "C:\\Windows\\System32");
    if (str->contains("%ProgramData%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("%ProgramData%", 0, Qt::CaseInsensitive), 13, "C:\\ProgramData");
    if (str->contains("%ProgramFiles(x86)%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("%ProgramFiles(x86)%", 0, Qt::CaseInsensitive), 19, "C:\\Program Files (x86)");
    if (str->contains("%ProgramFiles%", Qt::CaseInsensitive))
        *str = str->replace(str->indexOf("%ProgramFiles%", 0, Qt::CaseInsensitive), 14, "C:\\Program Files");
}

void InitLogOnTable(QTableWidget *table, QString reg)
{
    InitTableColumnWidth(table);
    int rowIndex = table->rowCount();
    DrawHeader(table, rowIndex, reg);

    QMap<QString, QString> *regMap = new QMap<QString, QString>;
    HKEY_PATH *item = new HKEY_PATH;
    getHkeyAndPath(reg, item);
    myReadReg(item->hKey, item->path, regMap);

    QMapIterator<QString, QString> it(*regMap);

    while(it.hasNext())
    {
        it.next();
//        qDebug() << it.key() << ":" << it.value();
        rowIndex++;
        table->setRowCount(rowIndex + 1);
        QString key = it.key();
        QString imagePath = it.value();

        QTableWidgetItem* keyItem = new QTableWidgetItem(key);
        table->setItem(rowIndex, 1, keyItem);

        // Repair image path
        RepairString(&imagePath);

        DrawTable(table, rowIndex, imagePath, "");
    }

    delete regMap;
    delete item;
}

void InitServicesTable(QTableWidget *table, QString reg)
{
    InitTableColumnWidth(table);
    int rowIndex = table->rowCount();
    DrawHeader(table, rowIndex, reg);

    HKEY_PATH *item = new HKEY_PATH;
    getHkeyAndPath(reg, item);
    GROUP_KEY *group = new GROUP_KEY;
    group = GetGroupKeyValue(item->hKey, item->path);
//    qDebug() << group[0].length;

    for (int i = 0; i < group[0].length; i++)
    {
        QMap<QString, QString> *groupMap = new QMap<QString, QString>;

        QString groupName = QString::fromWCharArray(group[i].subKey);

        HKEY_PATH *groupItem = new HKEY_PATH;
        getHkeyAndPath(reg + "\\" + groupName, groupItem);
        ReadServices(item->hKey, groupItem->path, groupMap);

        QString imagePath = groupMap->value("ImagePath");
        QString description = groupMap->value("Description");
        QString start = groupMap->value("Start");
        QString type = groupMap->value("Type");

        RepairString(&imagePath);

        if (imagePath != "" && (type == "16" || type == "32") && (start == "0" || start == "1" || start == "2"))
        {

            rowIndex++;
            table->setRowCount(rowIndex + 1);

//            qDebug() << rowIndex << imagePath << groupName;
            QTableWidgetItem *nameItem = new QTableWidgetItem(groupName);
            table->setItem(rowIndex, 1, nameItem);

            DrawTable(table, rowIndex, imagePath, description);
        }
        delete groupItem;
        delete groupMap;
    }
    delete group;
    delete item;
}

void InitDriversTable(QTableWidget *table, QString reg)
{
    InitTableColumnWidth(table);
    int rowIndex = table->rowCount();
    DrawHeader(table, rowIndex, reg);

    HKEY_PATH *item = new HKEY_PATH;
    getHkeyAndPath(reg, item);
    GROUP_KEY *group = new GROUP_KEY;
    group = GetGroupKeyValue(item->hKey, item->path);
//    qDebug() << group[0].length;

    for (int i = 0; i < group[0].length; i++)
    {
        QMap<QString, QString> *groupMap = new QMap<QString, QString>;

        QString groupName = QString::fromWCharArray(group[i].subKey);

//        QString pathQstring = QString::fromStdString((std::string)item->path) + "\\" + QString::fromWCharArray(group[i].subKey);
//        LPCSTR path = pathQstring.toLocal8Bit().constData();

        HKEY_PATH *groupItem = new HKEY_PATH;
        getHkeyAndPath(reg + "\\" + groupName, groupItem);
//        LPCSTR path = ((std::string)item->path + "\\" + groupName.toStdString()).c_str();
        ReadServices(item->hKey, groupItem->path, groupMap);

        QString imagePath = groupMap->value("ImagePath");
        QString description = groupMap->value("Description");
        QString start = groupMap->value("Start");
        QString type = groupMap->value("Type");

        RepairString(&imagePath);

        if (imagePath != "" && (type == "1" || type == "2"))
        {
//            qDebug() << i << rowIndex << groupName;
            rowIndex++;
            table->setRowCount(rowIndex + 1);

//            qDebug() << rowIndex << imagePath << groupName;
            QTableWidgetItem *nameItem = new QTableWidgetItem(groupName);
            table->setItem(rowIndex, 1, nameItem);

            DrawTable(table, rowIndex, imagePath, description);
        }
        delete groupItem;
        delete groupMap;
    }
    delete group;
    delete item;
}

void InitTasksTable(QTableWidget *table)
{
    InitTableColumnWidth(table);
    int rowIndex = table->rowCount();

    QMap<QString, QString> *taskMap = new QMap<QString, QString>;

    table->setRowCount(rowIndex + 1);
    QTableWidgetItem* regItem = new QTableWidgetItem("Task Scheduler");
    regItem->setBackgroundColor(QColor(200, 200, 255));
    table->setItem(rowIndex, 0, regItem);
    table->setSpan(rowIndex, 0, 1, 6);

    ReadTasks(taskMap);

    QMapIterator<QString, QString> it(*taskMap);

    while(it.hasNext())
    {
        it.next();
        rowIndex++;
        table->setRowCount(rowIndex + 1);
        QString key = it.key();
        QString imagePath = it.value();

        QTableWidgetItem* keyItem = new QTableWidgetItem(key);
        table->setItem(rowIndex, 1, keyItem);

        // Repair image path
        RepairString(&imagePath);

        DrawTable(table, rowIndex, imagePath, "");
    }

    delete taskMap;
}

void InitKnownDllsTable(QTableWidget *table, QString reg)
{
    InitTableColumnWidth(table);
    int rowIndex = table->rowCount();
    DrawHeader(table, rowIndex, reg);

    QMap<QString, QString> *regMap = new QMap<QString, QString>;
    HKEY_PATH *item = new HKEY_PATH;
    getHkeyAndPath(reg, item);
    myReadReg(item->hKey, item->path, regMap);

    QMapIterator<QString, QString> it(*regMap);

    while(it.hasNext())
    {
        it.next();
//        qDebug() << it.key() << ":" << it.value();
        rowIndex++;
        table->setRowCount(rowIndex + 1);
        QString key = it.key();
        QString imagePath = it.value();

        QTableWidgetItem* keyItem = new QTableWidgetItem(key);
        table->setItem(rowIndex, 1, keyItem);

        imagePath = "C:\\Windows\\system32\\" + imagePath;

        DrawTable(table, rowIndex, imagePath, "");
    }

    delete regMap;
    delete item;
}

void getHkeyAndPath(QString reg, HKEY_PATH *item)
{
//    qDebug() << reg;
    if (reg.startsWith("HKLM"))
    {
        item->hKey = HKLM;
    }
    if (reg.startsWith("HKCU"))
    {
        item->hKey = HKCU;
    }
    QString tmpPath = reg.section("\\", 1, -1);
    item->path = tmpPath.toLocal8Bit().constData();
}

void myReadReg(HKEY hKey, LPCSTR path, QMap<QString, QString> *map)
{
    KEY_VALUE *list = new KEY_VALUE;
    list = GetKeyValue(hKey, path);
//    QDebug deb = qDebug();
//    deb << list[0].length << endl;
    for (int i = 0; i < list[0].length; i++)
    {
        QString key, value;
        for (int j = 0; j < sizeof(list[i].key); j++)
        {
            if (list[i].key[j] != 0)
            {
                key += (char)list[i].key[j];
            }
            else
            {
                break;
            }
        }

        for (int k = 0; k < sizeof(list[i].value); k += 2)
        {
            if (list[i].value[k] != 0)
            {
                value += (char)(list[i].value[k]+list[i].value[k+1]);
            }
            else
            {
                break;
            }
        }
//        deb << i << key << value << endl;
        map->insert(key, value);
    }
    delete list;
}

void ReadServices(HKEY hKey, LPCSTR path, QMap<QString, QString> *map)
{
    KEY_VALUE *list = new KEY_VALUE;
    list = GetKeyValue(hKey, path);

    for (int i = 0; i < list[0].length; i++)
    {
        QString key, value;
        for (int j = 0; j < sizeof(list[i].key); j++)
        {
            if (list[i].key[j] != 0)
            {
                key += (char)list[i].key[j];
            }
            else
            {
                break;
            }
        }
        if (key == "ImagePath" || key == "Description")
        {
            for (int k = 0; k < sizeof(list[i].value); k += 2)
            {
                if (list[i].value[k] != 0)
                {
                    value += (char)(list[i].value[k]+list[i].value[k+1]);
                }
                else
                {
                    break;
                }
            }
            map->insert(key, value);
        }
        if (key == "Type" || key == "Start")
        {
            value = QString::number(list[i].value[0]);
            map->insert(key, value);
        }
    }
    delete list;
}

void ReadTasks(QMap<QString, QString> *map)
{  
    TASK *list = new TASK [1024];
    GetAllTasks(list);
    for (int i = 1; i <= list[0].length; i++)
    {
        QString key, value;
        key = QString(QLatin1String(list[i].taskName));
        value = QString(QLatin1String(list[i].imagePath));
        if (value == "")
        {
            continue;
        }
        else
        {
            map->insert(key, value);
        }
    }
    delete[] list;
}

void DrawHeader(QTableWidget *table, int rowIndex, QString reg)
{
    table->setRowCount(rowIndex + 1);
    QTableWidgetItem* regItem = new QTableWidgetItem(reg);
    regItem->setBackgroundColor(QColor(200, 200, 255));
    table->setItem(rowIndex, 0, regItem);
    table->setSpan(rowIndex, 0, 1, 6);
}

void DrawTable(QTableWidget *table, int rowIndex, QString imagePath, QString description)
{
    table->setRowHeight(rowIndex, 30);
    QTableWidgetItem* imagePathItem = new QTableWidgetItem(imagePath);
    table->setItem(rowIndex, 4, imagePathItem);

    // get create time
    QFileInfo fileInfo(imagePath);
    QDateTime fileCreateDateTime = fileInfo.created();
    QString fileCreateTime = QObject::tr("%1").arg(fileCreateDateTime.toString("yyyy-MM-dd hh:mm:ss"));
    QTableWidgetItem* timeStampItem = new QTableWidgetItem(fileCreateTime);
    table->setItem(rowIndex, 5, timeStampItem);

    // get file icon
    QFileIconProvider fileIcon;
    QIcon icon = fileIcon.icon(fileInfo);
    QTableWidgetItem* iconItem = new QTableWidgetItem;
    iconItem->setIcon(icon);
    table->setItem(rowIndex, 0, iconItem);

    // get description
    QString *buf = new QString;
    if (description.startsWith("@"))
    {
        GetDllDescription(description, buf);
    }
    else
    {
        GetFileDescription(imagePath, buf);
    }
    QTableWidgetItem* descriptionItem = new QTableWidgetItem(*buf);
    table->setItem(rowIndex, 2, descriptionItem);

    *buf = "";

    // check verification
    VerifyEmbeddedSignature(imagePath.toStdWString().c_str(), buf);
    QString isVerified = *buf;

    *buf = "";
    // get publisher
    GetSignaturePublisher(char2TCHAR(QString2char(imagePath)), buf);
    QString publisher = isVerified + " " + *buf;
    QTableWidgetItem* publisherItem = new QTableWidgetItem(publisher);
    table->setItem(rowIndex, 3, publisherItem);

    delete buf;

    if (isVerified == "(Not Verified)")
    {
        for (int i = 0; i < 6; i++)
        {
            table->item(rowIndex, i)->setBackgroundColor(QColor(255, 200, 200));
        }
    }
}

void ShowDetail(int row, int column, QTableWidget *table, Ui::MainWindow *ui)
{
    QString text = table->item(row, column)->text();
    if (!text.startsWith("HKLM") && !text.startsWith("HKCU") && !text.startsWith("Task Scheduler"))
    {
        QIcon icon = table->item(row, 0)->icon();
        QString name = table->item(row, 1)->text();
        QString description = table->item(row, 2)->text();
        QString publisher = table->item(row, 3)->text();
        QString imagePath = table->item(row, 4)->text();
        QString timeStamp = table->item(row, 5)->text();
        ui->pushButton->setIcon(icon);
        ui->label_name->setText(name);
        ui->label_des->setText(description);
        ui->label_sig->setText(publisher);
        ui->label_path->setText(imagePath);
        ui->label_time->setText(timeStamp);
    }
}

void MainWindow::on_tableWidget_logOn_cellClicked(int row, int column)
{
    ShowDetail(row, column, ui->tableWidget_logOn, ui);
}

void MainWindow::on_tableWidget_services_cellClicked(int row, int column)
{
    ShowDetail(row, column, ui->tableWidget_services, ui);
}

void MainWindow::on_tableWidget_drivers_cellClicked(int row, int column)
{
    ShowDetail(row, column, ui->tableWidget_drivers, ui);
}

void MainWindow::on_tableWidget_tasks_cellClicked(int row, int column)
{
    ShowDetail(row, column, ui->tableWidget_tasks, ui);
}

void MainWindow::on_tableWidget_knownDLLs_cellClicked(int row, int column)
{
    ShowDetail(row, column, ui->tableWidget_knownDLLs, ui);
}
