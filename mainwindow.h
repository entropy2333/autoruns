#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QWidget>
#include <QSettings>
#include <QtWidgets>
#include <QHeaderView>
#include <QObject>
#include <stdio.h>
#include <QProcess>
#include <QDebug>
#include <windows.h>
#include <winreg.h>
#include <winver.h>
#include <QMap>
#include <QMapIterator>
#include <QString>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_listWidget_currentRowChanged(int currentRow);

    void on_tableWidget_logOn_cellClicked(int row, int column);
    void on_tableWidget_services_cellClicked(int row, int column);
    void on_tableWidget_drivers_cellClicked(int row, int column);
    void on_tableWidget_tasks_cellClicked(int row, int column);
    void on_tableWidget_knownDLLs_cellClicked(int row, int column);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
