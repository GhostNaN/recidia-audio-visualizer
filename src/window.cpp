#include <string>
#include <vector>

#include <QApplication>
#include <QWheelEvent>
#include <QVulkanInstance>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

#include <qt_window.h>
#include <recidia.h>

using namespace std;

MainWindow::MainWindow(VulkanWindow *vulkan_window) {

    this->setAttribute(Qt::WA_TranslucentBackground);

    QWidget *wrapper = QWidget::createWindowContainer(vulkan_window);
    this->setCentralWidget(wrapper);
}

void VulkanWindow::wheelEvent(QWheelEvent *event) {
    int mouseDir = event->angleDelta().y();

    if (mouseDir > 0) {
        if (settings->data.height_cap > 1) {
            settings->data.height_cap /= 1.25;

            if (settings->data.height_cap < 1)
                settings->data.height_cap = 1;
        }
    }
    else {
        if (settings->data.height_cap < settings->data.HEIGHT_CAP.MAX) {
            settings->data.height_cap *= 1.25;

            if (settings->data.height_cap > settings->data.HEIGHT_CAP.MAX)
                settings->data.height_cap = settings->data.HEIGHT_CAP.MAX;
        }
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    (void) event;
    this->centralWidget()->resize(1000, 501);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    plot_data->width = event->size().width();
    plot_data->height = event->size().height();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    string keyString = event->text().toStdString();
    char key = keyString.c_str()[0];
    change_setting_by_key(settings, key);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    (void) event;
    exit(EXIT_SUCCESS);
}

int init_gui(int argc, char *argv[], recidia_data *data, recidia_settings *settings) {
    QApplication app(argc, argv);

    QVulkanInstance inst;

//    inst.setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
    inst.create();
    VulkanWindow *vulkanWindow = new VulkanWindow;
    vulkanWindow->plot_data = data;
    vulkanWindow->settings = settings;

    vulkanWindow->setVulkanInstance(&inst);

    MainWindow mainWindow(vulkanWindow);
    mainWindow.setWindowTitle("ReCidia");
    mainWindow.plot_data = data;
    mainWindow.settings = settings;

    mainWindow.resize(1000, 500);
    mainWindow.show();

    return app.exec();
}

int display_audio_devices(vector<string> devices, vector<uint> pulse_indexes, vector<uint> port_indexes) {
    const char *argv[] = {"", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;
    QApplication app(argc, const_cast<char **>(argv));

    QDialog *dialog = new QDialog();

    QListWidget *pulseList = new QListWidget();
    QListWidget *portList = new QListWidget();

    QObject::connect(pulseList, &QListWidget::currentRowChanged,
                     [=]() { portList->clearSelection(); } );

    QObject::connect(portList, &QListWidget::currentRowChanged,
                     [=]() { pulseList->clearSelection(); } );

    QVBoxLayout *layout = new QVBoxLayout();

    uint i = 0;
    int d = 0;

    if (pulse_indexes.size()) {
        d = 1;
        QLabel *pulseLabel = new QLabel("PulseAudio Devices");
        pulseLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pulseLabel);

        layout->addWidget(pulseList);

        pulseList->addItem("Default Output");
    }
    for (i=0; i < pulse_indexes.size(); i++) {
        pulseList->addItem(QString::fromStdString(devices[i]));
    }

    if (port_indexes.size()) {
        QLabel *portLabel = new QLabel("PortAudio Devices");
        portLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(portLabel);

        layout->addWidget(portList);
    }
    for (uint j=0; j < port_indexes.size(); j++) {
        portList->addItem(QString::fromStdString(devices[i+j]));
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *okButton = new QPushButton("Ok");
    QObject::connect(okButton, &QPushButton::released,
                     [=]() { dialog->done(0); } );
    buttonLayout->addWidget(okButton);

    QPushButton *cancelButton = new QPushButton("Cancel");
    QObject::connect(cancelButton, &QPushButton::released,
                     [=]() { exit(0); } );
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->resize(600, 450);
    dialog->exec();


    int index = 0;
    if (pulseList->selectedItems().size() != 0)
        index = pulseList->currentRow();
    else if (portList->selectedItems().size() != 0)
        index = portList->currentRow() + pulse_indexes.size()+d;

//    app.exit(0);
//    delete dialog; delete devList; delete layout; delete okButton; delete cancelButton;

    return index;
}
