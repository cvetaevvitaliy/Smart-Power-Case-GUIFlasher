#ifndef MAIN_APP_H
#define MAIN_APP_H
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QString>
#include <QFileDialog>
#include <QSpacerItem>
#include <QTextBrowser>
#include <QThread>

extern "C" {
#include "hidapi.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#define SECTOR_SIZE                     1024
#define HID_TX_SIZE                     65
#define HID_RX_SIZE                     9

#define VID                             0x1209
#define PID                             0xBEBA
#define FIRMWARE_VER                    0x0300
}


class Flash_Device : public QThread
{
    Q_OBJECT

public:
    explicit Flash_Device(const char *file);
    void run();
signals:
void Flash_Device_Sent_Message(QString str);
void NeedStartFlashDevice(const char *);
void NeedUpdateProgressBar(int);
void NeedSetRangeProgressBar(int, int);

private:

    const char *open_file;
    int usb_write(hid_device *device, uint8_t *buffer, int len);
    long int filesize( FILE *fp );

};



class Main_App : public QWidget
{
    Q_OBJECT

public:
    Main_App(QWidget *parent = 0);
    void Main_App_Show();


private:
    QWidget *main_window;
    QPushButton *button_open;
    QPushButton *button_flash;
    QSpacerItem *spaser_left;
    QSpacerItem *spaser_center;
    QSpacerItem *spaser_right;

    QProgressBar *progress_bar;

    QHBoxLayout *layout_H;
    QVBoxLayout *layout_V;

    QString open_file;

    QTextBrowser *text_browser;

    Flash_Device *thread;

private slots:
    void OpenFile();
    void FlashDevice();
    void UpdateProgressBar(int);
    void SetRangeProgressBar(int, int);
    void UpdateMessage(QString str);

private:



signals:
    void NeedStartFlashDevice(const char *);
    void NeedUpdateProgressBar(int);
    void NeedSetRangeProgressBar(int, int);


};


#endif // MAIN_APP_H
