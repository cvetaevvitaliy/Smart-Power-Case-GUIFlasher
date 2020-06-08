#include "main_app.h"
#include <QtDebug>


Main_App::Main_App(QWidget *parent) : QWidget(parent)
{
    main_window = new QWidget;
    main_window->setWindowTitle("Smart Power Flasher v.1.0");
    main_window->setFixedSize(350,100);

    button_open = new QPushButton("&Open");
    button_flash = new QPushButton("&Flash");
    button_flash->setEnabled(false);

    progress_bar = new QProgressBar;
    progress_bar->setTextVisible(false);

    layout_H = new QHBoxLayout;
    layout_V = new QVBoxLayout;

    spaser_left = new QSpacerItem(30, 40,QSizePolicy::Minimum);
    spaser_center = new QSpacerItem(50, 40,QSizePolicy::Minimum);
    spaser_right = new QSpacerItem(30, 40,QSizePolicy::Minimum);

    layout_H->addItem(spaser_left);
    layout_H->addWidget(button_open);
    layout_H->addItem(spaser_center);
    layout_H->addWidget(button_flash);
    layout_H->addItem(spaser_right);
    layout_V->addLayout(layout_H);
    layout_V->addWidget(progress_bar);

    setLayout(layout_V);
    main_window->setLayout(layout_V);

    QObject::connect(button_open,SIGNAL(clicked()),this,SLOT(OpenFile()));
    QObject::connect(button_flash,SIGNAL(clicked()),this,SLOT(FlashDevice()));

}

void Main_App::Main_App_Show(){

    main_window->show();

}


void Main_App::FlashDevice(){

    qDebug() << open_file;
    QByteArray ba = open_file.toLocal8Bit();
    const char *c_str = ba.data();

    StartFlashDevice(c_str);

}

void Main_App::OpenFile(){

    //open_file = QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.bin");
    open_file = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Binary Files (*.bin)"));
    button_flash->setEnabled(true);
    qDebug() << open_file;

}

void Main_App::UpdateProgressBar(int num){

    progress_bar->setValue(num);

}

extern "C" {
int Main_App::StartFlashDevice(const char *file){

    uint8_t page_data[SECTOR_SIZE];
    uint8_t hid_tx_buf[HID_TX_SIZE];
    uint8_t hid_rx_buf[HID_RX_SIZE];
    uint8_t CMD_RESET_PAGES[8] = {'B', 'T', 'L', 'D', 'C', 'M', 'D', 0x00};
    uint8_t CMD_REBOOT_MCU[8] = {'B', 'T', 'L', 'D', 'C', 'M', 'D', 0x01};
    hid_device *handle = NULL;
    size_t read_bytes;
    FILE *firmware_file = NULL;
    int error = 0;
    uint32_t n_bytes = 0;
    int i;
    setbuf(stdout, NULL);

    firmware_file = fopen(file, "rb");
    if (!firmware_file) {
        printf("> Error opening firmware file: %s\n", file);
        return error;
    }
    uint32_t file_size = filesize(firmware_file);
    printf("File Size = %d\n", file_size);
    progress_bar->setRange(1024,file_size);

    hid_init();

        printf("> Searching for [%04X:%04X] device...\n", VID, PID);

        struct hid_device_info *devs, *cur_dev;
        uint8_t valid_hid_devices = 0;

        for (i = 0; i < 30; i++) { //Try up to 10 times to open the HID device.
            devs = hid_enumerate(VID, PID);
            cur_dev = devs;
            while (cur_dev) { //Search for valid HID Bootloader USB devices
                if ((cur_dev->vendor_id == VID) && (cur_dev->product_id = PID)) {
                    valid_hid_devices++;
                    if (cur_dev->release_number < FIRMWARE_VER) { // check version BootLoader
                        printf("\nError - Please update BootLoader to the latest version (v3.00+)");
                        goto exit;
                    }
                }
                cur_dev = cur_dev->next;
            }
            hid_free_enumeration(devs);
            printf("#");
            sleep(1);
            if (valid_hid_devices > 0) break;
        }
        if (valid_hid_devices == 0) {
            printf("\nError - [%04X:%04X] device is not found :(", VID, PID);
            error = 1;
            goto exit;
        }

        handle = hid_open(VID, PID, NULL);

        if (i == 10 && handle != NULL) {
            printf("\n> Unable to open the [%04X:%04X] device.\n", VID, PID);
            error = 1;
            goto exit;
        }

        printf("\n> [%04X:%04X] device is found !\n", VID, PID);
        sleep(2);

        // Send RESET PAGES command to put HID bootloader in initial stage...
        memset(hid_tx_buf, 0, sizeof(hid_tx_buf)); //Fill the hid_tx_buf with zeros.
        memcpy(&hid_tx_buf[1], CMD_RESET_PAGES, sizeof(CMD_RESET_PAGES));

        printf("> Sending erase Flash command...\n");

        // Flash is unavailable when writing to it, so USB interrupt may fail here
        if (!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
            printf("> Error while sending erase Flash command.\n");
            error = 1;
            goto exit;
        }
        sleep(2);
        memset(hid_tx_buf, 0, sizeof(hid_tx_buf));

        // Send Firmware File data
        printf("> Start flashing firmware to Flash...\n");
        sleep(1);

        memset(page_data, 0, sizeof(page_data));
        read_bytes = fread(page_data, 1, sizeof(page_data), firmware_file);


        while (read_bytes > 0) {

            for (int i = 0; i < SECTOR_SIZE; i += HID_TX_SIZE - 1) {
                memcpy(&hid_tx_buf[1], page_data + i, HID_TX_SIZE - 1);

                if ((i % 1024) == 0) {
                    printf(".");

                }

                // Flash is unavailable when writing to it, so USB interrupt may fail here
                if (!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
                    printf("> Error while flashing firmware data to Flash.\n");
                    error = 1;
                    goto exit;
                }
                n_bytes += (HID_TX_SIZE - 1);
                usleep(500);
            }

            printf(" %d Bytes\n", n_bytes);

            UpdateProgressBar(n_bytes);
            //progress_bar->setValue(n_bytes);

            do {
                hid_read(handle, hid_rx_buf, 9);
                usleep(500);
            } while (hid_rx_buf[7] != 0x02);

            memset(page_data, 0, sizeof(page_data));
            read_bytes = fread(page_data, 1, sizeof(page_data), firmware_file);
        }

        UpdateProgressBar(file_size);
        button_flash->setEnabled(false);
        printf("\n> Done!\n");

        // Send CMD_REBOOT_MCU command to reboot the microcontroller...
        memset(hid_tx_buf, 0, sizeof(hid_tx_buf));
        memcpy(&hid_tx_buf[1], CMD_REBOOT_MCU, sizeof(CMD_REBOOT_MCU));

        printf("> Sending reboot command...\n");

        // Flash is unavailable when writing to it, so USB interrupt may fail here
        if (!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
            printf("> Error while sending reboot command.\n");
        }

        exit:
        if (handle) {
            hid_close(handle);
        }

        hid_exit();

        if (firmware_file) {
            fclose(firmware_file);
        }

        printf("> Finish\n");

        return error;


}

int Main_App::usb_write(hid_device *device, uint8_t *buffer, int len){
    int retries = 20;
    int retval;

    while(((retval = hid_write(device, buffer, len)) < len) && --retries) {
      if(retval < 0) {
        usleep(100 * 1000); // No data has been sent here. Delay and retry.
      } else {
        return 0; // Partial data has been sent. Firmware will be corrupted. Abort process.
      }
    }

    if(retries <= 0) {
      return 0;
    }

    return 1;
}

long int Main_App::filesize( FILE *fp ){
    long int save_pos, size_of_file;

    save_pos = ftell( fp );
    fseek( fp, 0L, SEEK_END );
    size_of_file = ftell( fp );
    fseek( fp, save_pos, SEEK_SET );
    return( size_of_file );
  }

}

